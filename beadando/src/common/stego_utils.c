#include "stego_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================================
 * stego_frame
 *
 * Builds the raw bit-stream payload:
 *   [byte0..3 = LE uint32 message length] [message bytes]
 *
 * The result is what gets embedded, 1 bit per carrier channel byte.
 * ===================================================================== */
uint8_t* stego_frame(const StegoMessage* msg, size_t* framed_len)
{
    *framed_len = 4 + msg->length;
    uint8_t* buf = (uint8_t*)malloc(*framed_len);
    if (!buf) return NULL;

    uint32_t len32 = (uint32_t)msg->length;
    buf[0] = (uint8_t)( len32        & 0xFF);
    buf[1] = (uint8_t)((len32 >>  8) & 0xFF);
    buf[2] = (uint8_t)((len32 >> 16) & 0xFF);
    buf[3] = (uint8_t)((len32 >> 24) & 0xFF);
    memcpy(buf + 4, msg->data, msg->length);

    return buf;
}

/* =====================================================================
 * stego_check_capacity
 * ===================================================================== */
int stego_check_capacity(const Image* img, const StegoMessage* msg)
{
    /* Each carrier channel byte holds one payload bit.
       We need (4 + msg->length) payload bytes → × 8 carrier bytes. */
    size_t bits_needed   = (4 + msg->length) * 8;
    size_t carrier_bytes = (size_t)img->width * img->height * img->channels;

    if (bits_needed > carrier_bytes) {
        fprintf(stderr,
                "[stego] Message too large: need %zu carrier bytes, have %zu\n",
                bits_needed, carrier_bytes);
        return -1;
    }
    return 0;
}