#include "vector.h"

#include <stdio.h>
#include <stdlib.h>

static void vector_fill_seq(Vector* v, float start)
{
    int i;
    for (i = 0; i < v->size; ++i)
        v->data[i] = start + (float)i;
}

static void vector_print(const char* label, const Vector* v, int max_print)
{
    int i, limit = (v->size < max_print) ? v->size : max_print;
    printf("%s [size=%d]: ", label, v->size);
    for (i = 0; i < limit; ++i)
        printf("%.1f ", v->data[i]);
    if (v->size > max_print) printf("...");
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

    Vector* a      = vector_create(n);
    Vector* b      = vector_create(n);
    Vector* result = vector_create(n);

    if (!a || !b || !result) { fprintf(stderr, "OOM\n"); return 1; }

    vector_fill_seq(a, 1.0f);
    vector_fill_seq(b, 0.5f);

    if (add_vectors(a, b, result) != 0) {
        fprintf(stderr, "Vector addition failed\n");
        return 1;
    }

    vector_print("a     ", a,      8);
    vector_print("b     ", b,      8);
    vector_print("result", result, 8);

    vector_free(a);
    vector_free(b);
    vector_free(result);
    return 0;
}