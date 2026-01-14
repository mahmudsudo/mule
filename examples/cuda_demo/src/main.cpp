#include <iostream>
#include "../include/kernel.h"

int main() {
    std::cout << "Starting CUDA demo..." << std::endl;
    run_kernel();
    std::cout << "CUDA demo finished." << std::endl;
    return 0;
}
