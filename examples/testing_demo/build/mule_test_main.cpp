#include "../include/mule_test.h"
#include <iostream>
int main() {
    int passed = 0;
    int failed = 0;
    for (const auto& test : mule::get_tests()) {
        try {
            test.func();
            std::cout << "[PASS] " << test.name << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << test.name << ": " << e.what() << std::endl;
            failed++;
        }
    }
    std::cout << "Result: " << passed << " passed, " << failed << " failed." << std::endl;
    return failed > 0 ? 1 : 0;
}
