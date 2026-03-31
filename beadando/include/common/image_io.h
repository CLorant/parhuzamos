#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#include "stego_types.h"

/*
 * Load a binary PPM (P6) file into img.
 * img->pixels is malloc'd; call image_free() when done.
 * Returns 0 on success, -1 on error.
 */
int image_load_ppm(const char* path, Image* img);

/*
 * Save img to a binary PPM (P6) file.
 * Returns 0 on success, -1 on error.
 */
int image_save_ppm(const char* path, const Image* img);

/*
 * Allocate a new Image (pixels are uninitialized).
 * Returns 0 on success, -1 on allocation failure.
 */
int image_alloc(Image* img, int width, int height, int channels);

/*
 * Free the pixel buffer (safe to call multiple times).
 */
void image_free(Image* img);

/*
 * Deep-copy src into dst (fresh malloc for pixels).
 * Returns 0 on success, -1 on failure.
 */
int image_copy(Image* dst, const Image* src);

/*
 * Write a synthetic binary PPM filled with deterministic pseudo-random
 * pixels derived from seed.  Useful for benchmark carrier images.
 * Returns 0 on success, -1 on error.
 */
int image_generate_synthetic(const char* path, int width, int height,
                              unsigned int seed);

#endif /* IMAGE_IO_H */