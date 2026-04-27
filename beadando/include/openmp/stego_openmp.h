#ifndef STEGO_OPENMP_H
#define STEGO_OPENMP_H

#include "common/stego_types.h"

/*
 * Embed msg into img using LSB steganography, parallelised with OpenMP.
 * img->pixels is modified in-place.
 *
 * num_threads: number of OMP threads (0 = use OMP_NUM_THREADS / default).
 * Returns 0 on success, -1 if msg is too large for the carrier.
 */
int stego_encode_omp(Image* img, const StegoMessage* msg, int num_threads);

/*
 * Extract the hidden message from img using OpenMP.
 * msg->data is malloc'd; call stego_message_free() when done.
 *
 * num_threads: same semantics as stego_encode_omp.
 * Returns 0 on success, -1 on error (corrupt header or capacity mismatch).
 */
int stego_decode_omp(const Image* img, StegoMessage* msg, int num_threads);

#endif /* STEGO_OPENMP_H */