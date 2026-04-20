#include "benchmark.h"
#include "image_io.h"
#include "stego_types.h"
#include "stego_utils.h"
#include "stego_openmp.h"
#include "stego_opencl.h"
#include "run_cl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(_WIN32)
#  include <windows.h>
double get_time(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}
#else
#  include <sys/time.h>
double get_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}
#endif

/* -----------------------------------------------------------------------
 * Build a synthetic test message: `len` bytes of repeating ASCII pattern.
 * ----------------------------------------------------------------------- */
static StegoMessage make_test_message(size_t len)
{
    StegoMessage m;
    m.length = len;
    m.data   = (uint8_t*)malloc(len);
    for (size_t i = 0; i < len; i++)
        m.data[i] = (uint8_t)('A' + (i % 26));
    return m;
}

/* -----------------------------------------------------------------------
 * Time a single encode or decode pass, returning elapsed seconds.
 * carrier is NOT modified (a fresh copy is made for encode each trial).
 * ----------------------------------------------------------------------- */

typedef enum { OP_ENCODE, OP_DECODE } Op;

static double time_omp(Op op, const Image* carrier,
                        const StegoMessage* msg,
                        const Image* stego,   /* used by decode */
                        int p)
{
    double start, end;

    if (op == OP_ENCODE) {
        Image tmp;
        image_copy(&tmp, carrier);
        start = get_time();
        stego_encode_omp(&tmp, msg, p);
        end = get_time();
        image_free(&tmp);
    } else {
        StegoMessage out = {NULL, 0};
        start = get_time();
        stego_decode_omp(stego, &out, p);
        end = get_time();
        stego_message_free(&out);
    }
    return end - start;
}

static double time_ocl(Op op, CLContext* ctx,
                        const Image* carrier,
                        const StegoMessage* msg,
                        const Image* stego)
{
    double start, end;

    if (op == OP_ENCODE) {
        Image tmp;
        image_copy(&tmp, carrier);
        start = get_time();
        stego_encode_ocl(ctx, &tmp, msg);
        end = get_time();
        image_free(&tmp);
    } else {
        StegoMessage out = {NULL, 0};
        start = get_time();
        stego_decode_ocl(ctx, stego, &out);
        end = get_time();
        stego_message_free(&out);
    }
    return end - start;
}

/* -----------------------------------------------------------------------
 * Average over `trials` runs.
 * ----------------------------------------------------------------------- */
static double avg_time_omp(Op op, const Image* carrier,
                            const StegoMessage* msg, const Image* stego,
                            int p, int trials)
{
    double sum = 0.0;
    for (int t = 0; t < trials; t++)
        sum += time_omp(op, carrier, msg, stego, p);
    return sum / trials;
}

static double avg_time_ocl(Op op, CLContext* ctx,
                            const Image* carrier,
                            const StegoMessage* msg, const Image* stego,
                            int trials)
{
    double sum = 0.0;
    for (int t = 0; t < trials; t++)
        sum += time_ocl(op, ctx, carrier, msg, stego);
    return sum / trials;
}

/* =====================================================================
 * run_benchmark
 * ===================================================================== */
int run_benchmark(const BenchmarkConfig* cfg)
{
    FILE* f = fopen(cfg->csv_path, "w");
    if (!f) {
        fprintf(stderr, "[bench] Cannot open '%s' for writing\n", cfg->csv_path);
        return -1;
    }

    /* CSV header */
    fprintf(f,
        "n,p,"
        "omp_encode,ocl_encode,"
        "omp_decode,ocl_decode,"
        "S_omp_encode,E_omp_encode,S_ocl_encode,"
        "S_omp_decode,E_omp_decode,S_ocl_decode\n");
    /* cols:
       1   2
       3           4
       5           6
       7             8              9
       10            11             12
    */

    /* Initialise one shared OpenCL context for the entire benchmark */
    CLContext cl_ctx;
    if (cl_init(&cl_ctx) != 0) {
        fprintf(stderr, "[bench] OpenCL init failed – OCL columns will be -1\n");
        /* continue with OCL disabled rather than aborting */
    }

    for (int ni = 0; ni < cfg->n_count; ni++) {
        int side = cfg->n_widths[ni];           /* square image: side × side */
        long n   = (long)side * side;           /* pixel count (printed as n) */

        /* ---- Generate carrier image ---- */
        char carrier_path[256];
        snprintf(carrier_path, sizeof(carrier_path),
                 "data/samples/bench_%dx%d.ppm", side, side);
        image_generate_synthetic(carrier_path, side, side, (unsigned)side);

        Image carrier;
        if (image_load_ppm(carrier_path, &carrier) != 0) {
            fprintf(stderr, "[bench] Could not load carrier %s\n", carrier_path);
            continue;
        }

        /* ---- Build test message (75 % of capacity, max 4 MB) ---- */
        size_t cap = stego_capacity_bytes(&carrier);
        size_t msg_len = cap * 3 / 4;
        if (msg_len > 4u * 1024 * 1024) msg_len = 4u * 1024 * 1024;
        StegoMessage msg = make_test_message(msg_len);

        /* ---- Build the stego image (OCL encode; single authoritative copy) ---- */
        Image stego;
        image_copy(&stego, &carrier);
        stego_encode_omp(&stego, &msg, 1);   /* use p=1 OMP for the reference */

        /* ---- Warmup OCL (JIT compilation is cached after first run) ---- */
        {
            Image tmp; image_copy(&tmp, &carrier);
            stego_encode_ocl(&cl_ctx, &tmp, &msg);
            image_free(&tmp);
            StegoMessage out = {NULL,0};
            stego_decode_ocl(&cl_ctx, &stego, &out);
            stego_message_free(&out);
        }

        /* ---- Measure OCL (does not depend on p, measured once per n) ---- */
        double t_ocl_enc = avg_time_ocl(OP_ENCODE, &cl_ctx,
                                         &carrier, &msg, &stego,
                                         cfg->trials);
        double t_ocl_dec = avg_time_ocl(OP_DECODE, &cl_ctx,
                                         &carrier, &msg, &stego,
                                         cfg->trials);

        /* ---- Baseline: OMP at p=1 (used for all speedup/efficiency) ---- */
        double t_omp_enc_p1 = avg_time_omp(OP_ENCODE, &carrier, &msg,
                                            &stego, 1, cfg->trials);
        double t_omp_dec_p1 = avg_time_omp(OP_DECODE, &carrier, &msg,
                                            &stego, 1, cfg->trials);

        double S_ocl_enc = (t_ocl_enc > 0.0) ? t_omp_enc_p1 / t_ocl_enc : 0.0;
        double S_ocl_dec = (t_ocl_dec > 0.0) ? t_omp_dec_p1 / t_ocl_dec : 0.0;

        /* ---- Sweep p values ---- */
        for (int pi = 0; pi < cfg->p_count; pi++) {
            int p = cfg->p_values[pi];

            double t_omp_enc, t_omp_dec;
            if (p == 1) {
                t_omp_enc = t_omp_enc_p1;
                t_omp_dec = t_omp_dec_p1;
            } else {
                t_omp_enc = avg_time_omp(OP_ENCODE, &carrier, &msg,
                                          &stego, p, cfg->trials);
                t_omp_dec = avg_time_omp(OP_DECODE, &carrier, &msg,
                                          &stego, p, cfg->trials);
            }

            double S_omp_enc = (t_omp_enc > 0.0) ? t_omp_enc_p1 / t_omp_enc : 0.0;
            double E_omp_enc = (p > 0) ? S_omp_enc / p : 0.0;
            double S_omp_dec = (t_omp_dec > 0.0) ? t_omp_dec_p1 / t_omp_dec : 0.0;
            double E_omp_dec = (p > 0) ? S_omp_dec / p : 0.0;

            fprintf(f,
                "%ld,%d,"
                "%.6f,%.6f,"
                "%.6f,%.6f,"
                "%.4f,%.4f,%.4f,"
                "%.4f,%.4f,%.4f\n",
                n, p,
                t_omp_enc, t_ocl_enc,
                t_omp_dec, t_ocl_dec,
                S_omp_enc, E_omp_enc, S_ocl_enc,
                S_omp_dec, E_omp_dec, S_ocl_dec);

            printf("[bench] n=%ld p=%d | enc: OMP=%.4fs OCL=%.4fs | "
                   "dec: OMP=%.4fs OCL=%.4fs\n",
                   n, p, t_omp_enc, t_ocl_enc, t_omp_dec, t_ocl_dec);
        }

        stego_message_free(&msg);
        image_free(&carrier);
        image_free(&stego);
    }

    fclose(f);
    printf("[bench] Results saved to: %s\n", cfg->csv_path);

    cl_cleanup(&cl_ctx);

    /* ---- Invoke gnuplot ---- */
    if (cfg->plot_enabled) {
        /* Encode plots:  col_omp=3, col_ocl=4, col_S_omp=7, col_S_ocl=9 */
        /* Decode plots:  col_omp=5, col_ocl=6, col_S_omp=10, col_S_ocl=12 */
        typedef struct { const char* op; int col_omp; int col_S_omp; } PlotEntry;
        PlotEntry entries[] = {
            { "encoding",   3, 7  },
            { "decoding", 5, 10 },
        };

        for (int e = 0; e < 2; e++) {
            char out_n[256], out_p[256], cmd[512];

            snprintf(out_n, sizeof(out_n),
                     "data/plots/%s_n.png", entries[e].op);
            snprintf(out_p, sizeof(out_p),
                     "data/plots/%s_p.png", entries[e].op);

            snprintf(cmd, sizeof(cmd),
                     "gnuplot -c data/plot_n.plt \"%s\" \"%s\" \"%s\" %d %d",
                     entries[e].op, cfg->csv_path, out_n,
                     entries[e].col_omp, entries[e].col_S_omp);
            system(cmd);

            snprintf(cmd, sizeof(cmd),
                     "gnuplot -c data/plot_p.plt \"%s\" \"%s\" \"%s\" %d %d",
                     entries[e].op, cfg->csv_path, out_p,
                     entries[e].col_omp, entries[e].col_S_omp);
            system(cmd);
        }
    }

    return 0;
}

/* =====================================================================
 * benchmark_handle_args
 * ===================================================================== */
int benchmark_handle_args(int argc, char* argv[], BenchmarkConfig* cfg)
{
    typedef enum { NONE, N_MODE, P_MODE } Mode;
    Mode mode = NONE;

    cfg->plot_enabled = 1;
    cfg->n_count      = 0;
    cfg->p_count      = 0;
    cfg->trials       = 3;
    snprintf(cfg->csv_path, sizeof(cfg->csv_path),
             "data/results/performance.csv");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-noplot") == 0) {
            cfg->plot_enabled = 0;
        } else if (strncmp(argv[i], "n=", 2) == 0) {
            mode = N_MODE;
            cfg->n_widths[cfg->n_count++] = atoi(argv[i] + 2);
        } else if (strncmp(argv[i], "p=", 2) == 0) {
            mode = P_MODE;
            cfg->p_values[cfg->p_count++] = atoi(argv[i] + 2);
        } else if (strncmp(argv[i], "t=", 2) == 0) {
            cfg->trials = atoi(argv[i] + 2);
        } else {
            if (mode == N_MODE)
                cfg->n_widths[cfg->n_count++] = atoi(argv[i]);
            else if (mode == P_MODE)
                cfg->p_values[cfg->p_count++] = atoi(argv[i]);
            else {
                fprintf(stderr,
                        "[bench] Unknown argument '%s'. "
                        "Usage: n=<v>... p=<v>... t=<trials> -noplot\n",
                        argv[i]);
                return -1;
            }
        }
    }

    /* Defaults when the user omits n or p */
    if (cfg->n_count == 0) {
        int def[] = { 256, 512, 1024, 2048, 4096 };
        cfg->n_count = (int)(sizeof(def) / sizeof(def[0]));
        memcpy(cfg->n_widths, def, sizeof(def));
    }
    if (cfg->p_count == 0) {
        int def[] = { 1, 2, 4, 8 };
        cfg->p_count = (int)(sizeof(def) / sizeof(def[0]));
        memcpy(cfg->p_values, def, sizeof(def));
    }

    return 0;
}