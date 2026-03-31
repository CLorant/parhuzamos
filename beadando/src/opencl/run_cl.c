#include "run_cl.h"
#include "kernel_loader.h"

#include <stdio.h>
#include <stdlib.h>

/* -----------------------------------------------------------------------
 * Internal macro: jump to a label on OpenCL error, printing a message.
 * ----------------------------------------------------------------------- */
#define CL_CHECK(err, label, msg)                                        \
    do {                                                                  \
        if ((err) != CL_SUCCESS) {                                        \
            fprintf(stderr, "[OpenCL] %s (code %d)\n", (msg), (err));   \
            goto label;                                                   \
        }                                                                 \
    } while (0)

/* -----------------------------------------------------------------------
 * Internal helper: print the build log for a failed clBuildProgram call.
 * ----------------------------------------------------------------------- */
static void print_build_log(cl_program program, cl_device_id device)
{
    size_t log_size;
    char*  log;

    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                          0, NULL, &log_size);
    log = (char*)malloc(log_size + 1);
    if (!log) return;

    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                          log_size, log, NULL);
    log[log_size] = '\0';
    fprintf(stderr, "[OpenCL] Build log:\n%s\n", log);
    free(log);
}

/* -----------------------------------------------------------------------
 * Internal helper: decide whether to upload host data for this buffer.
 * We upload when the buffer is readable by the kernel (READ_ONLY or
 * READ_WRITE) and the caller provided host data.
 * ----------------------------------------------------------------------- */
static int buffer_needs_upload(const CLBufferDesc* desc)
{
    if (!desc->host_ptr) return 0;
    return (desc->flags & CL_MEM_READ_ONLY)  ||
           (desc->flags & CL_MEM_READ_WRITE);
}

/* =====================================================================
 * cl_init
 * ===================================================================== */
int cl_init(CLContext* ctx)
{
    cl_int  err;
    cl_uint count;

    err = clGetPlatformIDs(1, &ctx->platform_id, &count);
    CL_CHECK(err, fail, "clGetPlatformIDs failed");

    err = clGetDeviceIDs(ctx->platform_id, CL_DEVICE_TYPE_GPU,
                         1, &ctx->device_id, &count);
    CL_CHECK(err, fail, "clGetDeviceIDs failed");

    ctx->context = clCreateContext(NULL, 1, &ctx->device_id,
                                   NULL, NULL, &err);
    CL_CHECK(err, fail, "clCreateContext failed");

    ctx->command_queue = clCreateCommandQueue(ctx->context,
                                              ctx->device_id, 0, &err);
    CL_CHECK(err, fail_ctx, "clCreateCommandQueue failed");

    return 0;

fail_ctx: clReleaseContext(ctx->context);
fail:     return -1;
}

/* =====================================================================
 * cl_cleanup
 * ===================================================================== */
void cl_cleanup(CLContext* ctx)
{
    clReleaseCommandQueue(ctx->command_queue);
    clReleaseContext(ctx->context);
    clReleaseDevice(ctx->device_id);
}

/* =====================================================================
 * cl_run_kernel  --  generic engine; nothing in here is task-specific
 * ===================================================================== */
int cl_run_kernel(CLContext*          ctx,
                  const CLKernelDesc* kd,
                  CLBufferDesc*       bufs,
                  int                 n_bufs,
                  CLArgBindFn         bind_args,
                  void*               user_data)
{
    cl_int     err;
    int        i, ret        = -1;
    int        loader_err;
    char*      source        = NULL;
    cl_program program       = NULL;
    cl_kernel  kernel        = NULL;
    cl_mem*    device_bufs   = NULL;

    /* ---- 1. Allocate the cl_mem handle array ---- */
    device_bufs = (cl_mem*)calloc((size_t)n_bufs, sizeof(cl_mem));
    if (!device_bufs) {
        fprintf(stderr, "[OpenCL] Out of memory for buffer handle array\n");
        goto cleanup;
    }

    /* ---- 2. Load and compile the kernel source ---- */
    source = load_kernel_source(kd->source_path, &loader_err);
    if (loader_err != 0) {
        fprintf(stderr, "[OpenCL] Could not load kernel source: %s\n",
                kd->source_path);
        goto cleanup;
    }

    program = clCreateProgramWithSource(ctx->context, 1,
                                        (const char**)&source, NULL, &err);
    CL_CHECK(err, cleanup, "clCreateProgramWithSource failed");

    err = clBuildProgram(program, 1, &ctx->device_id, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        print_build_log(program, ctx->device_id);
        goto cleanup;
    }

    kernel = clCreateKernel(program, kd->kernel_name, &err);
    CL_CHECK(err, cleanup, "clCreateKernel failed");

    /* ---- 3. Allocate device buffers and upload readable host data ---- */
    for (i = 0; i < n_bufs; ++i) {
        cl_mem_flags alloc_flags = bufs[i].flags;

        /* Add COPY_HOST_PTR automatically for readable buffers that have
           host data — avoids an explicit clEnqueueWriteBuffer. */
        if (buffer_needs_upload(&bufs[i]))
            alloc_flags |= CL_MEM_COPY_HOST_PTR;

        device_bufs[i] = clCreateBuffer(ctx->context,
                                        alloc_flags,
                                        bufs[i].size,
                                        buffer_needs_upload(&bufs[i])
                                            ? bufs[i].host_ptr : NULL,
                                        &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "[OpenCL] clCreateBuffer failed for buffer %d "
                            "(code %d)\n", i, err);
            goto cleanup;
        }
    }

    /* ---- 4. Bind kernel arguments (task-specific, done by callback) ---- */
    if (bind_args(kernel, device_bufs, n_bufs, user_data) != 0) {
        fprintf(stderr, "[OpenCL] Argument binding callback returned error\n");
        goto cleanup;
    }

    /* ---- 5. Enqueue the kernel ---- */
    err = clEnqueueNDRangeKernel(ctx->command_queue, kernel,
                                 kd->work_dim,
                                 NULL,                /* global offset */
                                 kd->global_size,
                                 kd->local_size,      /* may be NULL */
                                 0, NULL, NULL);
    CL_CHECK(err, cleanup, "clEnqueueNDRangeKernel failed");

    /* ---- 6. Read back output buffers (blocking on the last one) ---- */
    for (i = 0; i < n_bufs; ++i) {
        if (!bufs[i].read_back || !bufs[i].host_ptr) continue;

        /* Block only on the very last read-back to maximise overlap. */
        cl_bool blocking = (i == n_bufs - 1) ? CL_TRUE : CL_FALSE;

        err = clEnqueueReadBuffer(ctx->command_queue,
                                  device_bufs[i],
                                  blocking,
                                  0,
                                  bufs[i].size,
                                  bufs[i].host_ptr,
                                  0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "[OpenCL] clEnqueueReadBuffer failed for buffer "
                            "%d (code %d)\n", i, err);
            goto cleanup;
        }
    }

    /* If no buffer had read_back == 1, the queue may still be running;
       finish it so the caller can safely use device results via mapping etc. */
    clFinish(ctx->command_queue);

    ret = 0; /* success */

cleanup:
    /* Release device buffers in reverse order */
    for (i = n_bufs - 1; i >= 0; --i)
        if (device_bufs && device_bufs[i])
            clReleaseMemObject(device_bufs[i]);

    free(device_bufs);
    if (kernel)  clReleaseKernel(kernel);
    if (program) clReleaseProgram(program);
    free(source);
    return ret;
}