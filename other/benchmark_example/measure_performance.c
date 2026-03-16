#include "bst.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define NUM_SEARCHES 100000
#define MAX_TRIAL 3

#if defined(_WIN32)
    #include <windows.h>

    double get_time() {
        LARGE_INTEGER frequency, start;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
        return (double)start.QuadPart / frequency.QuadPart;
    }
#else
    #include <sys/time.h>

    double get_time() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
    }
#endif

void measure_sequential(int problem_size, double* times, int* keys, int* search_keys) {
    double start = get_time();
    radix_sort(keys, problem_size);
    Node* tree = build_balanced_tree_sequential(keys, 0, problem_size);
    times[0] = get_time() - start;

    start = get_time();
    for (int i = 0; i < NUM_SEARCHES; i++) {
        search_sequential(tree, search_keys[i]);
    }
    times[1] = get_time() - start;

    start = get_time();
    int* serialized = malloc(problem_size * sizeof(int));
    tree_to_sorted_array_sequential(tree, serialized, 0, problem_size);
    times[2] = get_time() - start;

    free(serialized);

    printf("Szekvenciális BST helyesség: %d\n", check_bst(tree));

    free_tree(tree);
}

void measure_parallel(int problem_size, int num_threads, double* times, int* keys, int* search_keys) {
    double start = get_time();
    radix_sort_parallel(keys, problem_size, num_threads);
    merge_chunks_parallel(keys, problem_size, num_threads);
    Node* tree = build_balanced_tree_parallel(keys, 0, problem_size, num_threads);
    times[0] = get_time() - start;
    
    start = get_time();
    search_parallel(tree, search_keys, num_threads, NUM_SEARCHES);
    times[1] = get_time() - start;
    
    start = get_time();
    int* serialized_par = malloc(problem_size * sizeof(int));
    tree_to_sorted_array_parallel(tree, serialized_par, 0, problem_size, num_threads);
    times[2] = get_time() - start;

    free(serialized_par);

    printf("Párhuzamos BST helyesség: %d\n", check_bst(tree));

    free_tree(tree);
}

void generate_keys(int* keys, int* search_keys, int n) {
    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        keys[i] = (int)(((uint64_t)rand() << 32) | rand());
    }

    for (int i = 0; i < NUM_SEARCHES; i++) {
        if (i % 2 == 0 && n > 0) {
            search_keys[i] = keys[rand() % n];
        }
        else {
            search_keys[i] = (int)(((uint64_t)rand() << 32) | rand());
        }
    }
}

double get_speedup(double seq_time, double par_time) {
    return (par_time != 0.0) ? seq_time / par_time : 0.0;
}

double get_efficiency(double speedup, int num_threads) {
    return (num_threads != 0) ? speedup / num_threads : 0.0;
}

void write_csv(char *filename, char *headers, char *pattern, int *n_values, int n_count, int *p_values, int p_count) {
    FILE *file = fopen(filename, "w");
    
    if (!file) {
        perror("Hiba a fájl megnyitásakor");
        exit(EXIT_FAILURE);
    }

    fprintf(file, headers);

    for (int i = 0; i < n_count; i++) {
        int n = n_values[i];
        int* keys = malloc(n * sizeof(int));
        int* search_keys = malloc(NUM_SEARCHES * sizeof(int));
        generate_keys(keys, search_keys, n);

        double seq_times[4];
        measure_sequential(n_values[i], seq_times, keys, search_keys);

        for (int j = 0; j < p_count; j++) {
            double sum_times[4] = {0.0};
            for (int k = 0; k < MAX_TRIAL; k++) {
                double par_times[4];

                measure_parallel(n_values[i], p_values[j], par_times, keys, search_keys);

                for (int m = 0; m < 4; m++) {
                    sum_times[m] += par_times[m];
                }
            }

            double avg_par_times[4];
            
            for (int k = 0; k < 4; k++) {
                avg_par_times[k] = sum_times[k] / MAX_TRIAL;
            }

            double speedups[3];
            double efficiencies[3];
            for (int i = 0; i < 3; i++) {
                speedups[i] = get_speedup(seq_times[i], avg_par_times[i]);
                efficiencies[i] = get_efficiency(speedups[i], p_values[j]);
            }

            fprintf(file, pattern,
                n_values[i], p_values[j],
                seq_times[0], avg_par_times[0],
                seq_times[1], avg_par_times[1],
                seq_times[2], avg_par_times[2],
                speedups[0], efficiencies[0],
                speedups[1], efficiencies[1],
                speedups[2], efficiencies[2]
            );
        }

        free(keys);
        free(search_keys);
    }

    fclose(file);
    printf("Eredmények mentve: %s\n", filename);
}

int handle_args(int argc, char *argv[], int* n_values, int* n_count, int* p_values, int* p_count, bool* plot_enabled) {
    enum { NONE, N_MODE, P_MODE } mode = NONE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-noplot") == 0) {
            *plot_enabled = false;
        }
        else if (strncmp(argv[i], "n=", 2) == 0) {
            mode = N_MODE;
            n_values[(*n_count)++] = atoi(argv[i] + 2);
        }
        else if (strncmp(argv[i], "p=", 2) == 0) {
            mode = P_MODE;
            p_values[(*p_count)++] = atoi(argv[i] + 2);
        }
        else {
            if (mode == N_MODE) {
                n_values[(*n_count)++] = atoi(argv[i]);
            }
            else if (mode == P_MODE) {
                p_values[(*p_count)++] = atoi(argv[i]);
            }
            else {
                fprintf(stderr, "Hiba: ismeretlen paraméter '%s'. Használható: n=, p=, -noplot\n", argv[i]);
                return EXIT_FAILURE;
            }
        }
    }

    if (*n_count == 0 || *p_count == 0) {
        int defaults_n[] = {2500000, 5000000, 10000000};
        int defaults_p[] = {1, 2, 4, 6, 8, 10};
        *n_count = sizeof(defaults_n) / sizeof(defaults_n[0]);
        *p_count = sizeof(defaults_p) / sizeof(defaults_p[0]);
        memcpy(n_values, defaults_n, sizeof(defaults_n));
        memcpy(p_values, defaults_p, sizeof(defaults_p));
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    char *filename = "data/performance_results.csv";
    char *headers = "n,p,seq_build,par_build,seq_search,par_search,seq_serialize,par_serialize,S_build,E_build,S_search,E_search,S_serialize,E_serialize\n";
    char *pattern = "%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n";

    int n_values[100], p_values[100];
    int n_count = 0;
    int p_count = 0;
    bool plot_enabled = true;
    
    if (handle_args(argc, argv, n_values, &n_count, p_values, &p_count, &plot_enabled) != 0) {
        return EXIT_FAILURE;
    }

    double start = get_time();
    write_csv(filename, headers, pattern, n_values, n_count, p_values, p_count);
    printf("Futási idő: %lf mp\n", get_time() - start);

    if (plot_enabled) {
        PlotData plots[] = {
            {filename, "1_build", 3, 9, "Faépítés"},
            {filename, "2_search", 5, 11, "Keresés"},
            {filename, "3_serialize", 7, 13, "Fasorosítás"},
        };

        for (int i = 0; i < 3; i++) {
            char output_n[128], output_p[128], command[256];
            
            snprintf(output_n, sizeof(output_n), "data/%s_n.png", plots[i].img_name);
            snprintf(output_p, sizeof(output_p), "data/%s_p.png", plots[i].img_name);

            snprintf(command, sizeof(command), 
                "gnuplot -c data/plot_n.plt \"%s\" \"%s\" \"%s\" %d %d",
                plots[i].plot_name, plots[i].csv_name, output_n, 
                plots[i].col_seq, plots[i].col_speedup);
            system(command);

            snprintf(command, sizeof(command), 
                "gnuplot -c data/plot_p.plt \"%s\" \"%s\" \"%s\" %d %d",
                plots[i].plot_name, plots[i].csv_name, output_p, 
                plots[i].col_seq, plots[i].col_speedup);
            system(command);
        }
    }

    return EXIT_SUCCESS;
}