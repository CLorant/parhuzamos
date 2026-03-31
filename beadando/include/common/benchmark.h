#ifndef BENCHMARK_H
#define BENCHMARK_H

/* ======================================================================
 * BenchmarkConfig  --  everything run_benchmark() needs
 * ====================================================================== */
typedef struct {
    int  n_widths[32];   /* image widths to test (square images: n×n) */
    int  n_count;
    int  p_values[32];   /* OMP thread counts to test */
    int  p_count;
    int  trials;         /* timed repetitions per (n, p) combination */
    char csv_path[256];
    int  plot_enabled;
} BenchmarkConfig;

/*
 * Run the full benchmark, write a CSV, and (if plot_enabled) call gnuplot.
 * Returns 0 on success.
 */
int run_benchmark(const BenchmarkConfig* cfg);

/*
 * Parse argc/argv into cfg.
 * Accepts: [n=<val> [val...]] [p=<val> [val...]] [-noplot]
 * Falls back to built-in defaults if n or p are not supplied.
 * Returns 0 on success, non-zero on bad arguments.
 */
int benchmark_handle_args(int argc, char* argv[], BenchmarkConfig* cfg);

/* Cross-platform wall-clock time in seconds */
double get_time(void);

#endif /* BENCHMARK_H */