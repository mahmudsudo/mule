#include "../../include/core/Utils.h"
#include <iostream>

namespace mule {
    bool command_exists(const std::string& cmd) {
#ifdef _WIN32
        std::string check = cmd + " --version > NUL 2>&1";
#else
        std::string check = cmd + " --version > /dev/null 2>&1";
#endif
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
#if defined(_MSC_VER)
        FILE* pipe = _popen(cmd, "r");
#else
        FILE* pipe = popen(cmd, "r");
#endif
        if (!pipe) return "";
        try {
            while (fgets(buffer, sizeof buffer, pipe) != NULL) {
                result += buffer;
            }
        } catch (...) {
#if defined(_MSC_VER)
            _pclose(pipe);
#else
            pclose(pipe);
#endif
            throw;
        }
#if defined(_MSC_VER)
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        if (!result.empty() && result.back() == '\n') result.pop_back();
        return result;
    }
}
