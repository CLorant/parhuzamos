/**
 * vector.cl  --  element-wise addition of two float arrays
 *
 * Each work-item handles exactly one element, so there are no
 * data dependencies between work-items.
 */
__kernel void add_vectors_kernel(__global const float* v1,
                                 __global const float* v2,
                                 __global       float* result,
                                 const          int    n)
{
    int i = get_global_id(0);

    /* Guard against the extra work-items created by rounding the
       global size up to a multiple of the local size. */
    if (i < n)
        result[i] = v1[i] + v2[i];
}
