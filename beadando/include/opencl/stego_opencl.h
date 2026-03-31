#ifndef STEGO_OPENCL_H
#define STEGO_OPENCL_H

#include "stego_types.h"
#include "run_cl.h"

/*
 * Embed msg into img on the GPU using LSB steganography.
 * img->pixels is modified in-place (written back from device after kernel).
 *
 * ctx must already be initialised with cl_init().
 * Returns 0 on success, -1 on error.
 */
int stego_encode_ocl(CLContext* ctx, Image* img, const StegoMessage* msg);

/*
 * Extract the hidden message from img on the GPU.
 * msg->data is malloc'd; call stego_message_free() when done.
 *
 * ctx must already be initialised with cl_init().
 * Returns 0 on success, -1 on error.
 */
int stego_decode_ocl(CLContext* ctx, const Image* img, StegoMessage* msg);

#endif /* STEGO_OPENCL_H */