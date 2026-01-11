#include <iostream>

int main() {
#ifdef TEST_DEFINE
    std::cout << "TEST_DEFINE is defined!" << std::endl;
#else
    std::cout << "TEST_DEFINE is NOT defined!" << std::endl;
#endif
    return 0;
}
