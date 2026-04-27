#include "common/stego_utils.h"
#include "openmp/stego_openmp.h"

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* =====================================================================
 * stego_encode_omp
 *
 * Embedding scheme:
 *   payload = stego_frame(msg)          (4-byte length prefix + data)
 *   pixel_byte[i] &= ~1                 clear LSB
 *   pixel_byte[i] |= (payload[i/8] >> (i%8)) & 1   set LSB to payload bit i
 *
 * Work is distributed across pixel bytes (each byte is independent).
 * ===================================================================== */
int stego_encode_omp(Image* img, const StegoMessage* msg, int num_threads)
{
    if (stego_check_capacity(img, msg) != 0)
        return -1;

    size_t framed_len;
    uint8_t* payload = stego_frame(msg, &framed_len);
    if (!payload) return -1;

    size_t total_bits = framed_len * 8;   /* = number of carrier bytes used */

    if (num_threads > 0)
        omp_set_num_threads(num_threads);

    /*
     * Each iteration i writes to a unique pixel byte img->pixels[i],
     * so there are no data races.
     */
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < total_bits; i++) {
        uint8_t bit = (payload[i >> 3] >> (i & 7)) & 1;
        img->pixels[i] = (img->pixels[i] & 0xFE) | bit;
    }

    free(payload);
    return 0;
}

/* =====================================================================
 * stego_decode_omp
 *
 * Pass 1 (serial): read 32 carrier bytes → recover 4-byte length prefix.
 * Pass 2 (parallel): each thread reconstructs a distinct output byte
 *   by collecting 8 consecutive LSBs. No data races because each
 *   iteration writes to a unique msg->data[byte_i].
 * ===================================================================== */
int stego_decode_omp(const Image* img, StegoMessage* msg, int num_threads)
{
    size_t total_carrier = (size_t)img->width * img->height * img->channels;

    /* ---- Pass 1: decode the 4-byte length prefix (32 carrier bytes) ---- */
    if (total_carrier < 32) {
        fprintf(stderr, "[stego/omp] Carrier image too small to hold a header\n");
        return -1;
    }

    uint8_t len_bytes[4] = {0};
    for (int i = 0; i < 32; i++) {
        uint8_t bit = img->pixels[i] & 1;
        len_bytes[i >> 3] |= (bit << (i & 7));
    }

    uint32_t len32 =  (uint32_t)len_bytes[0]
                   | ((uint32_t)len_bytes[1] <<  8)
                   | ((uint32_t)len_bytes[2] << 16)
                   | ((uint32_t)len_bytes[3] << 24);

    if (len32 == 0 || (size_t)(4 + len32) * 8 > total_carrier) {
        fprintf(stderr,
                "[stego/omp] Invalid embedded length %u "
                "(carrier can hold at most %zu bytes)\n",
                len32, stego_capacity_bytes(img));
        return -1;
    }

    /* ---- Pass 2: decode message bytes in parallel ---- */
    msg->length = (size_t)len32;
    msg->data   = (uint8_t*)malloc(len32);
    if (!msg->data) return -1;

    if (num_threads > 0)
        omp_set_num_threads(num_threads);

    /*
     * Each iteration writes to a unique msg->data[byte_i].
     * The inner loop over 8 bits is not parallelised; it stays serial
     * inside each thread's work unit.
     */
    #pragma omp parallel for schedule(static)
    for (size_t byte_i = 0; byte_i < (size_t)len32; byte_i++) {
        uint8_t val = 0;
        size_t  base = 32 + byte_i * 8;   /* skip the 32-bit length header */
        for (int b = 0; b < 8; b++) {
            val |= (img->pixels[base + b] & 1) << b;
        }
        msg->data[byte_i] = val;
    }

    return 0;
}