#include "math_utils.h"
#include "mule_test.h"
#include <iostream>

int main() {
    std::cout << "Running complex integration test..." << std::endl;
    if (add(10, 20) != 30) return 1;
    std::cout << "Integration test passed!" << std::endl;
    return 0;
}
