/*
 * OpenCL kernels for LSB steganography.
 *
 * Host code sets arguments as follows:
 *
 * encode_kernel:
 *   0 : __global uchar*   pixels   (read-write, carrier image bytes)
 *   1 : __global const uchar* payload (read-only, framed payload)
 *   2 : int                total_bits (number of LSBs to replace)
 *
 * decode_kernel:
 *   0 : __global const uchar* pixels  (read-only, stego image bytes)
 *   1 : __global uchar*       output  (write-only, decoded message bytes)
 *   2 : int                   bit_offset (first bit to read; 0 for header, 32 for message)
 *   3 : int                   num_bytes  (how many output bytes to produce)
 */

__kernel void encode_kernel(__global uchar* pixels,
                            __global const uchar* payload,
                            int total_bits)
{
    int i = get_global_id(0);
    if (i >= total_bits) return;

    // Extract the i-th payload bit: (payload[i/8] >> (i%8)) & 1
    uchar bit = (payload[i >> 3] >> (i & 7)) & 1;

    // Pixels[i] = (pixels[i] & 0xFE) | bit
    pixels[i] = (pixels[i] & 0xFE) | bit;
}

__kernel void decode_kernel(__global const uchar* pixels,
                            __global uchar* output,
                            int bit_offset,
                            int num_bytes)
{
    int byte_i = get_global_id(0);
    if (byte_i >= num_bytes) return;

    uchar val = 0;
    int base = bit_offset + byte_i * 8;   // first pixel index for this byte

    // Assemble 8 consecutive LSBs into one byte
    for (int b = 0; b < 8; b++) {
        val |= (pixels[base + b] & 1) << b;
    }

    output[byte_i] = val;
}