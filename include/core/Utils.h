#pragma once
#include <string>
#include <cstdlib>

namespace mule {
    bool command_exists(const std::string& cmd);
    void print_version();
    std::string get_exe_ext();
    std::string exec_cmd(const char* cmd);
    const std::string VERSION = "0.2.0-dev";
}
