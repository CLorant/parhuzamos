__kernel void add_vectors_kernel(__global const float* input,
                                 __global       float* result,
                                 const          int    n)
{
    int i = get_global_id(0);

    if (input[i] == 0) {
        result[i] = (input[i - 1] + input[i + 1]) / 2;
    }
}
