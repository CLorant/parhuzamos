#ifndef STEGO_TYPES_H
#define STEGO_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/* ======================================================================
 * Image  --  flat RGB pixel buffer (PPM P6 layout: R,G,B,R,G,B,...)
 * ====================================================================== */
typedef struct {
    uint8_t* pixels;   /* malloc'd; row-major, 3 bytes per pixel */
    int      width;
    int      height;
    int      channels; /* always 3 for PPM */
} Image;

/* ======================================================================
 * StegoMessage  --  the payload to hide / that was extracted
 *
 * Caller owns data; use stego_message_free() when done.
 * ====================================================================== */
typedef struct {
    uint8_t* data;
    size_t   length;   /* byte count; does NOT include the framing prefix */
} StegoMessage;

/* Maximum bytes that can be hidden: one bit per channel byte */
static inline size_t stego_capacity_bytes(const Image* img)
{
    return (size_t)(img->width * img->height * img->channels) / 8;
}

static inline void stego_message_free(StegoMessage* msg)
{
    free(msg->data);
    msg->data   = NULL;
    msg->length = 0;
}

#endif /* STEGO_TYPES_H */