#pragma once
#include "ConfigParser.h"
#include <string>

namespace mule {
    class Builder {
    public:
        static void build(const Config& config);
        static void run(const Config& config);
        static void clean();
        static CompilerType detect_compiler(std::string& out_cmd);
    };
}
