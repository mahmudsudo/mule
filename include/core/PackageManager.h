#pragma once
#include "ConfigParser.h"

namespace mule {
    class PackageManager {
    public:
        static std::vector<Dependency> fetch_dependencies(const std::vector<Dependency>& deps);
        static void write_lockfile(const std::vector<Dependency>& resolved);
    };
}
