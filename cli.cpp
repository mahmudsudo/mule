#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "cli.h"

namespace fs = std::filesystem;

const std::string MULE_VERSION = "0.1.0-alpha";

// --- Utility Functions ---

bool command_exists(const std::string& cmd) {
    std::string check = cmd + " --version > /dev/null 2>&1";
    return (std::system(check.c_str()) == 0);
}

void print_version() {
    std::cout << "mule version " << MULE_VERSION << std::endl;
}

void print_help() {
    std::cout << "Mule: A minimalist C++ build system and package manager\n\n"
              << "Usage: mule <command> [args]\n\n"
              << "Commands:\n"
              << "  new <name> [--lib]  Create a new project (default: bin)\n"
              << "  build               Compile project based on mule.toml\n"
              << "  run                 Build and execute the project binary\n"
              << "  clean               Remove build artifacts\n"
              << "  fetch               Download dependencies\n"
              << "  --version, -v       Show version info\n"
              << "  --help, -h          Show this help message\n";
}

// --- Configuration Parsing ---

std::map<std::string, std::map<std::string, std::string>> parse_mule_toml(const std::string &filename) {
    if (!fs::exists(filename)) {
        std::cerr << "Error: '" << filename << "' not found. Run 'mule new' first.\n";
        std::exit(1);
    }

    std::map<std::string, std::map<std::string, std::string>> config;
    std::ifstream file(filename);
    std::string line, current_section = "default";

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        size_t delim = line.find('=');
        if (delim != std::string::npos) {
            std::string key = line.substr(0, delim);
            std::string val = line.substr(delim + 1);
            key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
            val.erase(std::remove(val.begin(), val.end(), ' '), val.end());
            val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());
            config[current_section][key] = val;
        }
    }
    return config;
}

// --- Project Lifecycle ---

void create_template(const std::string &project_name, const std::string &type) {
    try {
        fs::path project_path = project_name;
        fs::create_directories(project_path / "src");
        fs::create_directories(project_path / "include");
        fs::create_directories(project_path / "tests");

        if (type == "bin") {
            std::ofstream main_file(project_path / "src" / "main.cpp");
            main_file << "#include <iostream>\n\nint main() {\n"
                      << "    std::cout << \"Hello from " << project_name << "!\" << std::endl;\n"
                      << "    return 0;\n}\n";
        } else {
            std::ofstream lib_hdr(project_path / "include" / (project_name + ".hpp"));
            lib_hdr << "#pragma once\n\nvoid hello();\n";
            std::ofstream lib_src(project_path / "src" / (project_name + ".cpp"));
            lib_src << "#include <iostream>\n#include \"" << project_name << ".hpp\"\n\n"
                    << "void hello() { std::cout << \"Lib " << project_name << " active!\" << std::endl; }\n";
        }

        std::ofstream config_file(project_path / "mule.toml");
        config_file << "[package]\nname = \"" << project_name << "\"\ntype = \"" << type << "\"\n"
                    << "version = \"0.1.0\"\nstandard = \"17\"\n";

        std::cout << "Successfully created " << type << " project: " << project_name << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Scaffolding failed: " << e.what() << std::endl;
    }
}

void fetch_deps() {
    auto config = parse_mule_toml("mule.toml");
    if (config.find("dependencies") == config.end()) return;

    fs::path deps_root = ".mule/deps";
    if (!fs::exists(deps_root)) fs::create_directories(deps_root);

    for (auto const& [lib_name, url] : config["dependencies"]) {
        fs::path lib_path = deps_root / lib_name;
        if (!fs::exists(lib_path)) {
            std::cout << "Downloading: " << lib_name << "...\n";
            std::string cmd = "git clone --depth 1 " + url + " " + lib_path.string();
            if (std::system(cmd.c_str()) != 0) std::cerr << "Failed to download " << lib_name << "\n";
        } else {
            std::cout << "Dependency " << lib_name << " is up to date.\n";
        }
    }
}

void run_build() {
    auto config = parse_mule_toml("mule.toml");
    std::string bin_name = config["package"]["name"];
    std::string type = config["package"].count("type") ? config["package"]["type"] : "bin";
    std::string std_ver = config["package"].count("standard") ? config["package"]["standard"] : "17";

    if (!command_exists("g++")) {
        std::cerr << "Error: g++ not found.\n";
        return;
    }

    fetch_deps();
    if (!fs::exists("build")) fs::create_directory("build");

    std::string inc_flags = "-Iinclude ";
    if (fs::exists(".mule/deps")) {
        for (const auto& entry : fs::directory_iterator(".mule/deps")) {
            if (entry.is_directory()) {
                inc_flags += "-I" + entry.path().string() + " ";
                if (fs::exists(entry.path() / "include")) 
                    inc_flags += "-I" + (entry.path() / "include").string() + " ";
            }
        }
    }

    std::vector<std::string> obj_files;
    for (const auto &entry : fs::recursive_directory_iterator("src")) {
        if (entry.path().extension() == ".cpp") {
            fs::path src = entry.path();
            fs::path obj = fs::path("build") / src.filename().replace_extension(".o");

            if (!fs::exists(obj) || fs::last_write_time(src) > fs::last_write_time(obj)) {
                std::string pic = (type == "dynamic-lib") ? "-fPIC " : "";
                std::string cmd = "g++ " + pic + "-std=c++" + std_ver + " -c " +
                                  src.string() + " -o " + obj.string() + " " + inc_flags;
                std::cout << "Compiling: " << src.filename() << "\n";
                if (std::system(cmd.c_str()) != 0) return;
            }
            obj_files.push_back(obj.string());
        }
    }

    // Linking Phase
    std::string link_cmd;
    if (type == "static-lib") {
        link_cmd = "ar rcs build/lib" + bin_name + ".a";
        for (const auto& o : obj_files) link_cmd += " " + o;
    } else if (type == "dynamic-lib") {
        link_cmd = "g++ -shared -o build/lib" + bin_name + ".so";
        for (const auto& o : obj_files) link_cmd += " " + o;
    } else {
        link_cmd = "g++ ";
        for (const auto& o : obj_files) link_cmd += o + " ";
        link_cmd += "-o build/" + bin_name;
    }

    std::cout << "Linking " << type << " [" << bin_name << "]...\n";
    std::system(link_cmd.c_str());
}

void run_project() {
    run_build();
    auto config = parse_mule_toml("mule.toml");
    std::string name = config["package"]["name"];
    fs::path bin = fs::path("build") / name;

    if (fs::exists(bin)) {
        std::cout << "--- Running " << name << " ---\n";
        std::system(("./" + bin.string()).c_str());
    }
}

void run_clean() {
    if (fs::exists("build")) {
        fs::remove_all("build");
        std::cout << "Build artifacts cleaned.\n";
    }
}

// --- CLI Entry Point ---

void cli_main(int argc, char *argv[]) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        print_help();
        return;
    }

    std::string cmd = argv[1];

    if (cmd == "--version" || cmd == "-v") {
        print_version();
    } else if (cmd == "new") {
        if (argc < 3) return;
        std::string type = (argc > 3 && std::string(argv[3]) == "--lib") ? "static-lib" : "bin";
        create_template(argv[2], type);
    } else if (cmd == "build") {
        run_build();
    } else if (cmd == "run") {
        run_project();
    } else if (cmd == "clean") {
        run_clean();
    } else if (cmd == "fetch") {
        fetch_deps();
    } else {
        print_help();
    }
}