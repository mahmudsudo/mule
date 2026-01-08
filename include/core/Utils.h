#pragma once
#include <string>
#include <cstdlib>

namespace mule {
    bool command_exists(const std::string& cmd);
    void print_version();
    const std::string VERSION = "0.2.0-dev";
}
