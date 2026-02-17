#include "vector.h"
#include "run_cl.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

Vector* vector_create(int size)
{
    Vector* v;
    if (size <= 0) return NULL;

    v = (Vector*)malloc(sizeof(Vector));
    if (!v) return NULL;

    v->data = (float*)calloc((size_t)size, sizeof(float));
    if (!v->data) { free(v); return NULL; }

    v->size = size;
    return v;
}

void vector_free(Vector* v)
{
    if (!v) return;
    free(v->data);
    free(v);
}

static void add_vectors_seq(const float* a, const float* b,
                             float* out, int n)
{
    int i;
    for (i = 0; i < n; ++i)
        out[i] = a[i] + b[i];
}

static int validate(const float* gpu, const float* seq, int n)
{
    const float tol = 1e-4f;
    int i;
    for (i = 0; i < n; ++i) {
        if (fabsf(gpu[i] - seq[i]) > tol) {
            fprintf(stderr,
                "[Validation] MISMATCH at [%d]: GPU=%.6f  SEQ=%.6f\n",
                i, gpu[i], seq[i]);
            return 0;
        }
    }
    return 1;
}

static int vector_add_bind_args(cl_kernel kernel,
                                cl_mem*   bufs,
                                int       n_bufs,
                                void*     user_data)
{
    int n = *(int*)user_data;
    cl_int err = CL_SUCCESS;

    (void)n_bufs;

    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufs[0]);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufs[1]);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufs[2]);
    err |= clSetKernelArg(kernel, 3, sizeof(int),    &n);

    return (err == CL_SUCCESS) ? 0 : -1;
}

int add_vectors(const Vector* v1, const Vector* v2, Vector* result)
{
    CLContext ctx;
    float*    seq_result = NULL;
    int       ret        = -1;
    int       n;

    if (!v1 || !v2 || !result) return -1;
    if (v1->size != v2->size || v1->size != result->size) {
        fprintf(stderr, "[add_vectors] Size mismatch (%d, %d, %d)\n",
                v1->size, v2->size, result->size);
        return -1;
    }

    n = v1->size;

    CLBufferDesc bufs[] = {
        { v1->data,     (size_t)n * sizeof(float), CL_MEM_READ_ONLY,  0 },
        { v2->data,     (size_t)n * sizeof(float), CL_MEM_READ_ONLY,  0 },
        { result->data, (size_t)n * sizeof(float), CL_MEM_WRITE_ONLY, 1 },
    };

    const size_t local_size  = 256;
    const size_t global_size = ((size_t)n + local_size - 1)
                               / local_size * local_size;

    CLKernelDesc kd = {
        .source_path = "kernels/vector.cl",
        .kernel_name = "add_vectors_kernel",
        .work_dim    = 1,
        .global_size = &global_size,
        .local_size  = &local_size,
    };

    /* ---- Run on GPU ---- */
    if (cl_init(&ctx) != 0) {
        fprintf(stderr, "[add_vectors] OpenCL init failed\n");
        return -1;
    }

    if (cl_run_kernel(&ctx, &kd, bufs, 3,
                      vector_add_bind_args, &n) != 0) {
        fprintf(stderr, "[add_vectors] GPU kernel failed\n");
        goto done;
    }

    /* ---- Validate against sequential reference ---- */
    seq_result = (float*)malloc((size_t)n * sizeof(float));
    if (!seq_result) { fprintf(stderr, "[add_vectors] OOM\n"); goto done; }

    add_vectors_seq(v1->data, v2->data, seq_result, n);

    if (!validate(result->data, seq_result, n)) {
        fprintf(stderr, "[add_vectors] Validation FAILED\n");
        goto done;
    }

    printf("[add_vectors] Validation passed\n");
    ret = 0;

done:
    free(seq_result);
    cl_cleanup(&ctx);
    return ret;
}