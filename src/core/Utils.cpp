#include "../../include/core/Utils.h"
#include <iostream>

namespace mule {
    bool command_exists(const std::string& cmd) {
        std::string check = cmd + " --version > /dev/null 2>&1";
        return (std::system(check.c_str()) == 0);
    }

    void print_version() {
        std::cout << "mule version " << VERSION << std::endl;
    }
}
