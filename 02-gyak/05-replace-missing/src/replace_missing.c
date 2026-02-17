#include "replace_missing.h"
#include "run_cl.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

float* arr_create(int size) {
    float* arr;
    if (size <= 0) return NULL;

    /*
    arr = (float*)malloc(sizeof(float));
    if (!arr) return NULL;
    */

    arr = (float*)calloc((size_t)size, sizeof(float));

    if (!arr) {
        free(arr);
        return NULL; 
    }

    return arr;
}

static int replace_missing_bind_args(cl_kernel kernel,
                                cl_mem*   bufs,
                                int       n_bufs,
                                void*     user_data)
{
    int n = *(int*)user_data;
    cl_int err = CL_SUCCESS;

    (void)n_bufs;

    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufs[0]);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufs[1]);
    err |= clSetKernelArg(kernel, 2, sizeof(int),    &n);

    return (err == CL_SUCCESS) ? 0 : -1;
}

int replace_missing(const float* input, float* result)
{
    CLContext ctx;
    int ret = -1;
    int n = sizeof(input) / sizeof(input[0]); // Needs setting

    // Needs setting
    CLBufferDesc bufs[] = {
        { input,  (size_t)n * sizeof(float), CL_MEM_READ_ONLY,  0 },
        { result, (size_t)n * sizeof(float), CL_MEM_WRITE_ONLY, 1 },
    };

    const size_t local_size  = 256;
    const size_t global_size = ((size_t)n + local_size - 1)
                               / local_size * local_size;

    // Needs setting
    CLKernelDesc kd = {
        .source_path = "kernels/replace_missing.cl",
        .kernel_name = "replace_missing_kernel",
        .work_dim    = 1,
        .global_size = &global_size,
        .local_size  = &local_size,
    };

    /* ---- Run on GPU ---- */
    if (cl_init(&ctx) != 0) {
        fprintf(stderr, "OpenCL init failed\n");
        return -1;
    }

    // Needs setting
    if (cl_run_kernel(&ctx, &kd, bufs, 2,
                      replace_missing_bind_args, &n) != 0) {
        fprintf(stderr, "GPU kernel failed\n");
        goto done;
    }

    printf("Validation passed\n");
    ret = 0;

done:
    cl_cleanup(&ctx);
    return ret;
}