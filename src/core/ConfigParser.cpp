#include "../../include/core/ConfigParser.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace mule {

    Config ConfigParser::parse(const std::string& filename) {
        Config config;
        std::map<std::string, std::map<std::string, std::string>> raw_config;
        std::ifstream file(filename);
        std::string line;
        std::string current_section = "default";

        if (!fs::exists(filename)) {
            std::cerr << "Error: Configuration file '" << filename << "' not found.\n"
                      << "Try running 'mule new <project_name>' first.\n";
            std::exit(1); 
        }

        while (std::getline(file, line)) {
            //  Basic cleaning
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty() || line[0] == '#')
                continue;

            //  Detect Section [section_name]
            if (line[0] == '[' && line.back() == ']') {
                current_section = line.substr(1, line.size() - 2);
                continue;
            }

            //  Detect Key-Value Pairs
            size_t delim = line.find('=');
            if (delim != std::string::npos) {
                std::string key = line.substr(0, delim);
                std::string val = line.substr(delim + 1);

                // Clean key/value whitespace
                key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
                val.erase(std::remove(val.begin(), val.end(), ' '), val.end());
                val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());

                raw_config[current_section][key] = val;
            }
        }

        // Map to struct
        if (raw_config.count("package")) {
            auto& pkg = raw_config["package"];
            if (pkg.count("name")) config.project_name = pkg["name"];
            if (pkg.count("version")) config.version = pkg["version"];
            if (pkg.count("standard")) config.standard = pkg["standard"];
        }

        if (raw_config.count("dependencies")) {
            for (const auto& [name, val] : raw_config["dependencies"]) {
                Dependency dep;
                dep.name = name;

                if (val.front() == '{' && val.back() == '}') {
                    // Parse inline table: { git = "...", tag = "..." }
                    std::string content = val.substr(1, val.size() - 2);
                    size_t pos = 0;
                    while (pos < content.length()) {
                        size_t eq = content.find('=', pos);
                        if (eq == std::string::npos) break;
                        
                        std::string k = content.substr(pos, eq - pos);
                        // Clean key
                        k.erase(std::remove(k.begin(), k.end(), ' '), k.end());
                        k.erase(std::remove(k.begin(), k.end(), ','), k.end());

                        size_t comma = content.find(',', eq);
                        if (comma == std::string::npos) comma = content.length();
                        
                        std::string v = content.substr(eq + 1, comma - (eq + 1));
                        // Clean value
                        v.erase(std::remove(v.begin(), v.end(), ' '), v.end());
                        v.erase(std::remove(v.begin(), v.end(), '\"'), v.end());
                        
                        if (k == "git") dep.git = v;
                        if (k == "tag") dep.tag = v;
                        if (k == "commit") dep.commit = v;
                        if (k == "path") dep.path = v;
                        
                        pos = comma + 1;
                    }
                } else {
                    // Legacy string format: name = "url"
                    dep.git = val;
                }
                config.dependencies.push_back(dep);
            }
        }

        return config;
    }
}
