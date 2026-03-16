__kernel void replace_missing_kernel(__global const float* input,
                                     __global float* output,
                                     const int n)
{
    int i = get_global_id(0);
    if (i < n) {
        if (input[i] == 0.0f && i > 0 && i < n-1) {
            output[i] = (input[i-1] + input[i+1]) / 2.0f;
        } else {
            output[i] = input[i];
        }
    }
}