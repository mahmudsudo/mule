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

    struct GeneratorConfig {
        std::string name;
        std::string input_extension;
        std::string output_extension;
        std::string command;
        std::string match_content;
    };

    struct BuildConfig {
        std::vector<std::string> lib_dirs;
        std::vector<std::string> libs;
        std::vector<std::string> include_dirs;
        std::vector<std::string> flags;
        std::vector<std::string> linker_flags;
    };

    struct QtConfig {
        bool enabled = false;
        std::vector<std::string> modules;
    };

    struct Config {
        std::string project_name;
        std::string version;
        std::string standard;
        std::string type; // "bin", "static-lib", "shared-lib"
        std::vector<Dependency> dependencies;
        BuildConfig build;
        std::vector<GeneratorConfig> generators;
        QtConfig qt;
    };

    class ConfigParser {
    public:
        static Config parse(const std::string& filename);
    };
}
