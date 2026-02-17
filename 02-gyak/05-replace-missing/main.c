#include "replace_missing.h"

#include <stdio.h>
#include <stdlib.h>

// Fill some idx with 0
void arr_fill_seq(float* arr, int size, float start)
{
    int i;
    for (i = 0; i < size; ++i) {
        arr[i] = (i % 12 == 0) ?
            (start + (float)i) : 0;
    } 
}

static void arr_print(const char* label, const float* arr, int size, int max_print)
{
    int i, limit = (size < max_print) ? size : max_print;
    printf("%s [size=%d]: ", label, size);
    for (i = 0; i < limit; ++i)
        printf("%.1f ", arr[i]);
    if (size > max_print) printf("...");
    printf("\n");
}

int main(int argc, char* argv[])
{
    int n = 1024;
    if (argc == 2) n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Usage: %s [n]\n", argv[0]);
        return 1;
    }

    float* input = arr_create(n);
    float* result = arr_create(n);

    if (!input || !result) { fprintf(stderr, "OOM\n"); return 1; }

    int size = sizeof(input) / sizeof(input[0]);

    arr_fill_seq(input, size, 1.0f);

    if (replace_missing(input, result) != 0) {
        fprintf(stderr, "Replace missing failed\n");
        return 1;
    }

    
    arr_print("input", input, size, 8);
    arr_print("result", result, size, 8);

    free(input);
    free(result);
    return 0;
}