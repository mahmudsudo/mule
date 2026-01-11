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

    std::string get_exe_ext() {
    #ifdef _WIN32
        return ".exe";
    #else
        return "";
    #endif
    }

    std::string exec_cmd(const char* cmd) {
        char buffer[128];
        std::string result = "";
        FILE* pipe = popen(cmd, "r");
        if (!pipe) return "";
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        pclose(pipe);
        if (!result.empty() && result.back() == '\n') result.pop_back();
        return result;
    }
}
