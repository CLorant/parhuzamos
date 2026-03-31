#include "image_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Internal: skip PPM comment lines (lines starting with '#').
 * The file position is left just after the last skipped comment.
 * ----------------------------------------------------------------------- */
static void skip_ppm_comments(FILE* f)
{
    int c;
    while ((c = fgetc(f)) == '#') {
        while ((c = fgetc(f)) != '\n' && c != EOF)
            ;
    }
    if (c != EOF)
        ungetc(c, f);
}

/* =====================================================================
 * image_load_ppm
 * ===================================================================== */
int image_load_ppm(const char* path, Image* img)
{
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[image_io] Cannot open '%s' for reading\n", path);
        return -1;
    }

    char magic[3];
    if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "[image_io] '%s' is not a binary PPM (P6) file\n", path);
        fclose(f);
        return -1;
    }

    skip_ppm_comments(f);
    int w, h, maxval;
    if (fscanf(f, "%d", &w) != 1) goto bad_header;
    skip_ppm_comments(f);
    if (fscanf(f, "%d", &h) != 1) goto bad_header;
    skip_ppm_comments(f);
    if (fscanf(f, "%d", &maxval) != 1) goto bad_header;

    /* Consume exactly one whitespace character before the binary data */
    fgetc(f);

    if (w <= 0 || h <= 0 || maxval != 255) {
        fprintf(stderr, "[image_io] Unsupported PPM header in '%s'\n", path);
        fclose(f);
        return -1;
    }

    img->width    = w;
    img->height   = h;
    img->channels = 3;
    img->pixels   = (uint8_t*)malloc((size_t)w * h * 3);
    if (!img->pixels) {
        fprintf(stderr, "[image_io] Out of memory\n");
        fclose(f);
        return -1;
    }

    if (fread(img->pixels, 1, (size_t)w * h * 3, f) != (size_t)(w * h * 3)) {
        fprintf(stderr, "[image_io] Truncated pixel data in '%s'\n", path);
        free(img->pixels);
        img->pixels = NULL;
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;

bad_header:
    fprintf(stderr, "[image_io] Malformed PPM header in '%s'\n", path);
    fclose(f);
    return -1;
}

/* =====================================================================
 * image_save_ppm
 * ===================================================================== */
int image_save_ppm(const char* path, const Image* img)
{
    FILE* f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[image_io] Cannot open '%s' for writing\n", path);
        return -1;
    }

    fprintf(f, "P6\n%d %d\n255\n", img->width, img->height);

    size_t n = (size_t)img->width * img->height * img->channels;
    if (fwrite(img->pixels, 1, n, f) != n) {
        fprintf(stderr, "[image_io] Write error for '%s'\n", path);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

/* =====================================================================
 * image_alloc
 * ===================================================================== */
int image_alloc(Image* img, int width, int height, int channels)
{
    img->width    = width;
    img->height   = height;
    img->channels = channels;
    img->pixels   = (uint8_t*)malloc((size_t)width * height * channels);
    if (!img->pixels) {
        fprintf(stderr, "[image_io] Out of memory\n");
        return -1;
    }
    return 0;
}

/* =====================================================================
 * image_free
 * ===================================================================== */
void image_free(Image* img)
{
    free(img->pixels);
    img->pixels = NULL;
}

/* =====================================================================
 * image_copy
 * ===================================================================== */
int image_copy(Image* dst, const Image* src)
{
    dst->width    = src->width;
    dst->height   = src->height;
    dst->channels = src->channels;
    size_t n      = (size_t)src->width * src->height * src->channels;
    dst->pixels   = (uint8_t*)malloc(n);
    if (!dst->pixels) {
        fprintf(stderr, "[image_io] Out of memory in image_copy\n");
        return -1;
    }
    memcpy(dst->pixels, src->pixels, n);
    return 0;
}

/* =====================================================================
 * image_generate_synthetic
 * Uses a simple LCG so output is deterministic for a given seed.
 * ===================================================================== */
int image_generate_synthetic(const char* path, int width, int height,
                              unsigned int seed)
{
    Image img;
    if (image_alloc(&img, width, height, 3) != 0)
        return -1;

    unsigned int s = seed;
    size_t n = (size_t)width * height * 3;
    for (size_t i = 0; i < n; i++) {
        s = s * 1664525u + 1013904223u;   /* Numerical Recipes LCG */
        img.pixels[i] = (uint8_t)(s >> 24);
    }

    int ret = image_save_ppm(path, &img);
    image_free(&img);
    return ret;
}