#pragma once
#include "ConfigParser.h"

namespace mule {
    class TestRunner {
    public:
        static void run_tests(const Config& config);
    };
}
