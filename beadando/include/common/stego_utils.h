#ifndef STEGO_UTILS_H
#define STEGO_UTILS_H

#include "common/stego_types.h"
#include <stdint.h>
#include <stddef.h>

/*
 * Build the framed payload that gets embedded into the carrier.
 * Layout: [4-byte LE length][message bytes]
 *
 * Returns a malloc'd buffer of length (4 + msg->length).
 * Caller must free() the result.
 * Returns NULL on allocation failure.
 */
uint8_t* stego_frame(const StegoMessage* msg, size_t* framed_len);

/*
 * Verify that msg fits inside img.
 * Returns 0 if it fits, -1 if the message is too large.
 */
int stego_check_capacity(const Image* img, const StegoMessage* msg);

#endif /* STEGO_UTILS_H */