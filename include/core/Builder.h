#pragma once
#include "ConfigParser.h"
#include <string>

namespace mule {
    enum class CompilerType { GCC, Clang, MSVC, Unknown };

    class Builder {
    public:
        static void build(const Config& config);
        static void run(const Config& config);
        static void clean();
        
    private:
        static CompilerType detect_compiler(std::string& out_cmd);
    };
}
