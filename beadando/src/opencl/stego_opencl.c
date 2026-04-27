#include "common/stego_utils.h"
#include "opencl/stego_opencl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define KERNEL_PATH  "kernels/steganography.cl"
#define LOCAL_SIZE   256u

typedef struct { int total_bits; } EncodeArgs;

static int encode_bind(cl_kernel kernel, cl_mem* bufs,
                       int n_bufs, void* user_data)
{
    (void)n_bufs;
    EncodeArgs* a = (EncodeArgs*)user_data;
    cl_int err = CL_SUCCESS;
    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufs[0]);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufs[1]);
    err |= clSetKernelArg(kernel, 2, sizeof(int),    &a->total_bits);
    return (err == CL_SUCCESS) ? 0 : -1;
}

typedef struct { int bit_offset; int num_bytes; } DecodeArgs;

static int decode_bind(cl_kernel kernel, cl_mem* bufs,
                       int n_bufs, void* user_data)
{
    (void)n_bufs;
    DecodeArgs* a = (DecodeArgs*)user_data;
    cl_int err = CL_SUCCESS;
    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufs[0]);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufs[1]);
    err |= clSetKernelArg(kernel, 2, sizeof(int),    &a->bit_offset);
    err |= clSetKernelArg(kernel, 3, sizeof(int),    &a->num_bytes);
    return (err == CL_SUCCESS) ? 0 : -1;
}

static size_t round_up(size_t n)
{
    return ((n + LOCAL_SIZE - 1) / LOCAL_SIZE) * LOCAL_SIZE;
}

int stego_encode_ocl(CLContext* ctx, Image* img, const StegoMessage* msg)
{
    if (stego_check_capacity(img, msg) != 0)
        return -1;

    size_t framed_len;
    uint8_t* payload = stego_frame(msg, &framed_len);
    if (!payload) return -1;

    int total_bits  = (int)(framed_len * 8);
    size_t gs       = round_up((size_t)total_bits);
    size_t ls       = LOCAL_SIZE;
    size_t img_size = (size_t)img->width * img->height * img->channels;

    CLBufferDesc bufs[] = {
        { img->pixels, img_size,       CL_MEM_READ_WRITE, 1 },
        { payload,     framed_len,     CL_MEM_READ_ONLY,  0 },
    };

    CLKernelDesc kd = {
        .source_path = KERNEL_PATH,
        .kernel_name = "encode_kernel",
        .work_dim    = 1,
        .global_size = &gs,
        .local_size  = &ls,
    };

    EncodeArgs args = { total_bits };
    int ret = cl_run_kernel(ctx, &kd, bufs, 2, encode_bind, &args);

    free(payload);
    return ret;
}

int stego_decode_ocl(CLContext* ctx, const Image* img, StegoMessage* msg)
{
    size_t img_size = (size_t)img->width * img->height * img->channels;
    if (img_size < 32) {
        fprintf(stderr, "[stego/ocl] Carrier too small to hold a header\n");
        return -1;
    }

    uint8_t len_bytes[4] = {0};
    {
        size_t gs = round_up(4);
        size_t ls = LOCAL_SIZE;

        CLBufferDesc bufs[] = {
            { (void*)img->pixels, img_size,    CL_MEM_READ_ONLY,  0 },
            { len_bytes,          4,           CL_MEM_WRITE_ONLY, 1 },
        };
        CLKernelDesc kd = {
            .source_path = KERNEL_PATH,
            .kernel_name = "decode_kernel",
            .work_dim    = 1,
            .global_size = &gs,
            .local_size  = &ls,
        };
        DecodeArgs args = { 0, 4 };
        if (cl_run_kernel(ctx, &kd, bufs, 2, decode_bind, &args) != 0)
            return -1;
    }

    uint32_t len32 =  (uint32_t)len_bytes[0]
                   | ((uint32_t)len_bytes[1] <<  8)
                   | ((uint32_t)len_bytes[2] << 16)
                   | ((uint32_t)len_bytes[3] << 24);

    if (len32 == 0 || (size_t)(4 + len32) * 8 > img_size) {
        fprintf(stderr,
                "[stego/ocl] Invalid embedded length %u\n", len32);
        return -1;
    }

    msg->length = (size_t)len32;
    msg->data   = (uint8_t*)malloc(len32);
    if (!msg->data) return -1;

    {
        size_t gs = round_up((size_t)len32);
        size_t ls = LOCAL_SIZE;

        CLBufferDesc bufs[] = {
            { (void*)img->pixels, img_size, CL_MEM_READ_ONLY,  0 },
            { msg->data,          len32,    CL_MEM_WRITE_ONLY, 1 },
        };
        CLKernelDesc kd = {
            .source_path = KERNEL_PATH,
            .kernel_name = "decode_kernel",
            .work_dim    = 1,
            .global_size = &gs,
            .local_size  = &ls,
        };
        DecodeArgs args = { 32, (int)len32 };
        if (cl_run_kernel(ctx, &kd, bufs, 2, decode_bind, &args) != 0) {
            stego_message_free(msg);
            return -1;
        }
    }

    return 0;
}