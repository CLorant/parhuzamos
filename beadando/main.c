#include "image_io.h"
#include "stego_types.h"
#include "stego_openmp.h"
#include "stego_opencl.h"
#include "run_cl.h"
#include "benchmark.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Print usage and exit.
 * ----------------------------------------------------------------------- */
static void usage(const char* prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s encode <carrier.ppm> <output.ppm> <message.txt>"
             " [--omp|--ocl] [--threads N]\n"
        "  %s decode <stego.ppm>   <output.txt>"
             " [--omp|--ocl] [--threads N]\n"
        "  %s bench  [n=<w>...] [p=<p>...] [t=<trials>] [-noplot]\n"
        "  %s gen    <width> <height> <output.ppm>\n"
        "\n"
        "Defaults: --omp, --threads 0 (OMP_NUM_THREADS / system default)\n",
        prog, prog, prog, prog);
    exit(EXIT_FAILURE);
}

/* -----------------------------------------------------------------------
 * Read an entire file into a StegoMessage.
 * msg->data is malloc'd; caller must free.
 * ----------------------------------------------------------------------- */
static int read_message_file(const char* path, StegoMessage* msg)
{
    FILE* f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    if (sz <= 0) { fclose(f); fprintf(stderr, "Empty message file\n"); return -1; }

    msg->length = (size_t)sz;
    msg->data   = (uint8_t*)malloc((size_t)sz);
    if (!msg->data) { fclose(f); return -1; }

    if (fread(msg->data, 1, (size_t)sz, f) != (size_t)sz) {
        perror("fread"); fclose(f); free(msg->data); return -1;
    }
    fclose(f);
    return 0;
}

/* -----------------------------------------------------------------------
 * Write msg bytes to a file.
 * ----------------------------------------------------------------------- */
static int write_message_file(const char* path, const StegoMessage* msg)
{
    FILE* f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }
    if (fwrite(msg->data, 1, msg->length, f) != msg->length) {
        perror("fwrite"); fclose(f); return -1;
    }
    fclose(f);
    return 0;
}

/* -----------------------------------------------------------------------
 * Parse optional backend/thread flags from argv[start..argc-1].
 * ----------------------------------------------------------------------- */
static void parse_backend_flags(int argc, char* argv[], int start,
                                 int* use_ocl, int* threads)
{
    *use_ocl = 0;
    *threads = 0;
    for (int i = start; i < argc; i++) {
        if (strcmp(argv[i], "--ocl") == 0) *use_ocl = 1;
        else if (strcmp(argv[i], "--omp") == 0) *use_ocl = 0;
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc)
            *threads = atoi(argv[++i]);
    }
}

/* =====================================================================
 * main
 * ===================================================================== */
int main(int argc, char* argv[])
{
    if (argc < 2) usage(argv[0]);

    /* ================================================================
     * bench
     * ================================================================ */
    if (strcmp(argv[1], "bench") == 0) {
        BenchmarkConfig cfg;
        if (benchmark_handle_args(argc - 1, argv + 1, &cfg) != 0)
            return EXIT_FAILURE;

        double t0 = get_time();
        int ret = run_benchmark(&cfg);
        printf("Total benchmark time: %.2f s\n", get_time() - t0);
        return ret ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    /* ================================================================
     * gen  <width> <height> <output.ppm>
     * ================================================================ */
    if (strcmp(argv[1], "gen") == 0) {
        if (argc < 5) usage(argv[0]);
        int w = atoi(argv[2]), h = atoi(argv[3]);
        if (w <= 0 || h <= 0) { fprintf(stderr, "Invalid dimensions\n"); return EXIT_FAILURE; }
        if (image_generate_synthetic(argv[4], w, h, (unsigned)(w * h)) != 0)
            return EXIT_FAILURE;
        printf("Generated %dx%d carrier: %s\n", w, h, argv[4]);
        return EXIT_SUCCESS;
    }

    /* ================================================================
     * encode  <carrier.ppm> <output.ppm> <message.txt> [flags]
     * ================================================================ */
    if (strcmp(argv[1], "encode") == 0) {
        if (argc < 5) usage(argv[0]);

        int use_ocl, threads;
        parse_backend_flags(argc, argv, 5, &use_ocl, &threads);

        Image carrier;
        if (image_load_ppm(argv[2], &carrier) != 0) return EXIT_FAILURE;

        StegoMessage msg;
        if (read_message_file(argv[4], &msg) != 0) { image_free(&carrier); return EXIT_FAILURE; }

        printf("Carrier: %dx%d (%zu bytes capacity)\n",
               carrier.width, carrier.height, stego_capacity_bytes(&carrier));
        printf("Message: %zu bytes | Backend: %s\n",
               msg.length, use_ocl ? "OpenCL" : "OpenMP");

        int ret;
        if (use_ocl) {
            CLContext ctx;
            if (cl_init(&ctx) != 0) { stego_message_free(&msg); image_free(&carrier); return EXIT_FAILURE; }
            ret = stego_encode_ocl(&ctx, &carrier, &msg);
            cl_cleanup(&ctx);
        } else {
            ret = stego_encode_omp(&carrier, &msg, threads);
        }

        if (ret == 0) {
            ret = image_save_ppm(argv[3], &carrier);
            if (ret == 0) printf("Output saved: %s\n", argv[3]);
        }

        stego_message_free(&msg);
        image_free(&carrier);
        return ret ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    /* ================================================================
     * decode  <stego.ppm> <output.txt> [flags]
     * ================================================================ */
    if (strcmp(argv[1], "decode") == 0) {
        if (argc < 4) usage(argv[0]);

        int use_ocl, threads;
        parse_backend_flags(argc, argv, 4, &use_ocl, &threads);

        Image stego;
        if (image_load_ppm(argv[2], &stego) != 0) return EXIT_FAILURE;

        printf("Stego image: %dx%d | Backend: %s\n",
               stego.width, stego.height, use_ocl ? "OpenCL" : "OpenMP");

        StegoMessage msg = {NULL, 0};
        int ret;
        if (use_ocl) {
            CLContext ctx;
            if (cl_init(&ctx) != 0) { image_free(&stego); return EXIT_FAILURE; }
            ret = stego_decode_ocl(&ctx, &stego, &msg);
            cl_cleanup(&ctx);
        } else {
            ret = stego_decode_omp(&stego, &msg, threads);
        }

        if (ret == 0) {
            ret = write_message_file(argv[3], &msg);
            if (ret == 0) printf("Decoded %zu bytes → %s\n", msg.length, argv[3]);
        }

        stego_message_free(&msg);
        image_free(&stego);
        return ret ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    usage(argv[0]);
    return EXIT_FAILURE;
}