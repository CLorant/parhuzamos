#ifndef VECTOR_H
#define VECTOR_H

typedef struct Vector {
    float* data;
    int size;
} Vector;

Vector* vector_create(int size);

void vector_free(Vector* v);

int add_vectors(const Vector* v1, const Vector* v2, Vector* result);

#endif /* VECTOR_H */