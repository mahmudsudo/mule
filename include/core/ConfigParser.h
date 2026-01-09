#pragma once
#include <string>
#include <map>
#include <vector>

namespace mule {
    struct Dependency {
        std::string name;
        std::string git; 
        std::string tag;
        std::string commit;
        std::string path;    
        std::string version; 
    };

    struct BuildConfig {
        std::vector<std::string> lib_dirs;
        std::vector<std::string> libs;
        std::vector<std::string> include_dirs;
        std::vector<std::string> flags;
    };

    struct Config {
        std::string project_name;
        std::string version;
        std::string standard;
        std::string type; // "bin", "static-lib", "shared-lib"
        std::vector<Dependency> dependencies;
        BuildConfig build;
    };

    class ConfigParser {
    public:
        static Config parse(const std::string& filename);
    };
}
