__kernel void hello_kernel(__global int* buffer, int n) {
    if (get_global_id(0) < n) {
        buffer[get_global_id(0)] = get_global_id(0) * 10;
    }
}