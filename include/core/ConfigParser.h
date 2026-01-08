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

    struct Config {
        std::string project_name;
        std::string version;
        std::string standard;
        std::vector<Dependency> dependencies;
    };

    class ConfigParser {
    public:
        static Config parse(const std::string& filename);
    };
}
