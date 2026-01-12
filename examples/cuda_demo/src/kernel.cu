#include "../include/kernel.h"
#include <iostream>

__global__ void hello_cuda() {
    printf("Hello from CUDA kernel!\n");
}

void run_kernel() {
    hello_cuda<<<1, 1>>>();
    cudaDeviceSynchronize();
}
