#ifndef RUN_CL_H
#define RUN_CL_H

#include <stddef.h>
#include <CL/cl.h>

/* ============================================================
 * CLContext  --  platform / device / context / queue
 * Completely task-agnostic; reuse as-is for any kernel.
 * ============================================================ */

typedef struct {
    cl_platform_id   platform_id;
    cl_device_id     device_id;
    cl_context       context;
    cl_command_queue command_queue;
} CLContext;

int  cl_init(CLContext* ctx);
void cl_cleanup(CLContext* ctx);


/* ============================================================
 * CLBufferDesc  --  describes one device buffer
 *
 * The runner allocates the cl_mem objects, optionally uploads
 * host data, runs the kernel, and optionally reads results back.
 * The caller never needs to touch cl_mem directly.
 * ============================================================ */

typedef struct {
    void*        host_ptr;   /* Host source (READ/RW) or destination (WRITE).
                                May be NULL for buffers used only on the device. */
    size_t       size;       /* Size in bytes.                                   */
    cl_mem_flags flags;      /* E.g. CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY,
                                CL_MEM_READ_WRITE.
                                CL_MEM_COPY_HOST_PTR is added automatically
                                when host_ptr != NULL and buffer is readable.   */
    int          read_back;  /* 1 = copy device buffer back to host_ptr after
                                the kernel finishes.  0 = leave on device.      */
} CLBufferDesc;


/* ============================================================
 * CLArgBindFn  --  the one task-specific piece
 *
 * Called by the runner after all buffers are allocated, just
 * before the kernel is enqueued.  The callback sets every
 * clSetKernelArg() call, because argument types and counts
 * differ per kernel and cannot be described generically.
 *
 * kernel    : the compiled kernel object
 * bufs      : device buffer handles, in the same order as the
 *             CLBufferDesc array passed to cl_run_kernel()
 * n_bufs    : number of entries in bufs[]
 * user_data : opaque pointer forwarded unchanged from cl_run_kernel()
 *
 * Return 0 on success, non-zero to abort the run.
 *
 * --- Example ---
 * For a kernel: __kernel void foo(__global float* a,
 *                                 __global float* b,
 *                                 __global float* c,
 *                                 const int n)
 *
 *   static int foo_bind(cl_kernel k, cl_mem* b, int nb, void* ud) {
 *       int n = *(int*)ud;
 *       clSetKernelArg(k, 0, sizeof(cl_mem), &b[0]);
 *       clSetKernelArg(k, 1, sizeof(cl_mem), &b[1]);
 *       clSetKernelArg(k, 2, sizeof(cl_mem), &b[2]);
 *       clSetKernelArg(k, 3, sizeof(int),    &n);
 *       return 0;
 *   }
 * ============================================================ */

typedef int (*CLArgBindFn)(cl_kernel kernel,
                           cl_mem*   bufs,
                           int       n_bufs,
                           void*     user_data);


/* ============================================================
 * CLKernelDesc  --  everything needed to build + launch a kernel
 * ============================================================ */

typedef struct {
    const char*   source_path;  /* Path to the .cl source file.           */
    const char*   kernel_name;  /* Entry-point name inside the .cl file.  */
    cl_uint       work_dim;     /* 1, 2, or 3.                            */
    const size_t* global_size;  /* Array of `work_dim` elements.          */
    const size_t* local_size;   /* Array of `work_dim` elements, or NULL
                                   for implementation-chosen size.        */
} CLKernelDesc;


/* ============================================================
 * cl_run_kernel  --  the generic, reusable engine
 *
 * ctx       : an initialised CLContext
 * kd        : kernel description (source path, name, work sizes)
 * bufs      : array of buffer descriptors
 * n_bufs    : number of entries in bufs[]
 * bind_args : callback that sets all kernel arguments
 * user_data : forwarded to bind_args unchanged (pass extra scalars etc.)
 *
 * Returns 0 on success, non-zero on any failure.
 *
 * What this function handles automatically:
 *   1. Load + compile the kernel source (prints build log on error).
 *   2. For each CLBufferDesc: allocate cl_mem; if host_ptr != NULL and
 *      the buffer is readable, upload the host data.
 *   3. Call bind_args (caller sets arguments).
 *   4. Enqueue NDRangeKernel.
 *   5. For each CLBufferDesc where read_back == 1: download to host_ptr.
 *   6. Release all cl_mem, kernel, and program objects.
 * ============================================================ */

int cl_run_kernel(CLContext*          ctx,
                  const CLKernelDesc* kd,
                  CLBufferDesc*       bufs,
                  int                 n_bufs,
                  CLArgBindFn         bind_args,
                  void*               user_data);

#endif /* RUN_CL_H */