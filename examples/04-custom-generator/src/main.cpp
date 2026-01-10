#include <iostream>
#include "version.proto.h" // The generated header

int main() {
    std::cout << "Application Version: " << VERSION << std::endl;
    return 0;
}
