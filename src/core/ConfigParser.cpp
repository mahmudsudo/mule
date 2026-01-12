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

            // Strip inline comments (basic quote-aware)
            bool in_quotes = false;
            size_t comment_pos = std::string::npos;
            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] == '\"') in_quotes = !in_quotes;
                if (line[i] == '#' && !in_quotes) {
                    comment_pos = i;
                    break;
                }
            }
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
            }

            if (line.empty()) continue;

            //  Detect Section [[section_name]] (Array of Tables)
            if (line.size() > 4 && line[0] == '[' && line[1] == '[' && line[line.size()-2] == ']' && line.back() == ']') {
                current_section = line.substr(2, line.size() - 4);
                static int gen_count = 0;
                if (current_section == "generator") {
                    current_section = "generator." + std::to_string(gen_count++);
                }
                continue;
            }

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

                // Clean key whitespace
                key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
                
                // Clean value leading/trailing whitespace and quotes
                val.erase(0, val.find_first_not_of(" \t\r\n"));
                val.erase(val.find_last_not_of(" \t\r\n") + 1);
                if (!val.empty() && (val.front() == '\"' || val.front() == '\'')) val.erase(0, 1);
                if (!val.empty() && (val.back() == '\"' || val.back() == '\'')) val.pop_back();

                // Basic unescape for strings
                std::string unescaped;
                for (size_t i = 0; i < val.length(); ++i) {
                    if (val[i] == '\\' && i + 1 < val.length()) {
                        char next = val[i+1];
                        if (next == '\"') unescaped += '\"';
                        else if (next == '\'') unescaped += '\'';
                        else if (next == 'n') unescaped += '\n';
                        else if (next == 't') unescaped += '\t';
                        else if (next == '\\') unescaped += '\\';
                        else unescaped += next;
                        i++;
                    } else {
                        unescaped += val[i];
                    }
                }
                val = unescaped;

                raw_config[current_section][key] = val;
            }
        }

        // Map to struct
        if (raw_config.count("package")) {
            auto& pkg = raw_config["package"];
            if (pkg.count("name")) config.project_name = pkg["name"];
            if (pkg.count("version")) config.version = pkg["version"];
            config.standard = pkg.count("standard") ? pkg["standard"] : "17";
            config.type = pkg.count("type") ? pkg["type"] : "bin";
        }

        auto parse_list = [](std::string val) {
            std::vector<std::string> res;
            if (val.empty()) return res;
            if (val.front() == '[' && val.back() == ']') {
                val = val.substr(1, val.size() - 2);
                size_t pos = 0;
                while (pos < val.length()) {
                    size_t comma = val.find(',', pos);
                    if (comma == std::string::npos) comma = val.length();
                    std::string item = val.substr(pos, comma - pos);
                    item.erase(0, item.find_first_not_of(" \t\""));
                    item.erase(item.find_last_not_of(" \t\"") + 1);
                    if (!item.empty()) res.push_back(item);
                    pos = comma + 1;
                }
            } else if (!val.empty()) {
                res.push_back(val);
            }
            return res;
        };

        if (raw_config.count("build")) {
            auto& bld = raw_config["build"];
            if (bld.count("lib_dirs")) config.build.lib_dirs = parse_list(bld["lib_dirs"]);
            if (bld.count("libs")) config.build.libs = parse_list(bld["libs"]);
            if (bld.count("include_dirs")) config.build.include_dirs = parse_list(bld["include_dirs"]);
            if (bld.count("flags")) config.build.flags = parse_list(bld["flags"]);
            if (bld.count("linker_flags")) config.build.linker_flags = parse_list(bld["linker_flags"]);
            if (bld.count("defines")) config.build.defines = parse_list(bld["defines"]);
        }

        if (raw_config.count("qt")) {
            auto& qt = raw_config["qt"];
            if (qt.count("enabled")) config.qt.enabled = (qt["enabled"] == "true");
            if (qt.count("modules")) config.qt.modules = parse_list(qt["modules"]);
        }

        if (raw_config.count("cuda")) {
            auto& cuda = raw_config["cuda"];
            if (cuda.count("enabled")) config.cuda.enabled = (cuda["enabled"] == "true");
        }

        // Parse generators
        for (const auto& [section, keys] : raw_config) {
            if (section.find("generator.") == 0) {
                GeneratorConfig gen;
                if (keys.count("name")) gen.name = keys.at("name");
                if (keys.count("input_extension")) gen.input_extension = keys.at("input_extension");
                if (keys.count("output_extension")) gen.output_extension = keys.at("output_extension");
                if (keys.count("command")) gen.command = keys.at("command");
                if (keys.count("match_content")) gen.match_content = keys.at("match_content");
                config.generators.push_back(gen);
            }
        }

        // Automatically add Qt generators if enabled
        if (config.qt.enabled) {
            // Standard Qt Modules if none specified
            if (config.qt.modules.empty()) {
                config.qt.modules = {"Core", "Gui", "Widgets"};
            }

            // MOC
            GeneratorConfig moc;
            moc.name = "qt-moc";
            moc.input_extension = ".h";
            moc.output_extension = ".moc.cpp";
            moc.command = "moc {input} -o {output}";
            moc.match_content = "Q_OBJECT";
            config.generators.push_back(moc);

            // MOC for GADGET
            moc.name = "qt-moc-gadget";
            moc.match_content = "Q_GADGET";
            config.generators.push_back(moc);

            // UIC
            GeneratorConfig uic;
            uic.name = "qt-uic";
            uic.input_extension = ".ui";
            uic.output_extension = ".ui.h";
            uic.command = "uic {input} -o {output}";
            config.generators.push_back(uic);

            // RCC
            GeneratorConfig rcc;
            rcc.name = "qt-rcc";
            rcc.input_extension = ".qrc";
            rcc.output_extension = ".qrc.cpp";
            rcc.command = "rcc {input} -o {output}";
            config.generators.push_back(rcc);
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
