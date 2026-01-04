#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "cli.h"

namespace fs = std::filesystem;

bool command_exists(const std::string& cmd) {
    std::string check = cmd + " --version > /dev/null 2>&1";
    return (std::system(check.c_str()) == 0);
}
const std::string MULE_VERSION = "0.1.0-alpha";

void print_version() {
    std::cout << "mule version " << MULE_VERSION << std::endl;
}

void create_template(const std::string &project_name)
{
    try
    {

        fs::path project_path = project_name;
        fs::create_directories(project_path / "src");
        fs::create_directories(project_path / "include");

        std::ofstream main_file(project_path / "src" / "main.cpp");
        main_file << "#include <iostream>\n\n"
                  << "int main() {\n"
                  << "    std::cout << \"Hello from " << project_name << "!\" << std::endl;\n"
                  << "    return 0;\n"
                  << "}\n";
        main_file.close();

        std::ofstream config_file(project_path / "mule.toml");
        config_file << "[package]\n"
                    << "name = \"" << project_name << "\"\n"
                    << "version = \"0.1.0\"\n"
                    << "standard = \"17\"\n";
        config_file.close();

        std::cout << "Successfully created project: " << project_name << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Scaffolding failed: " << e.what() << std::endl;
    }
}




std::map<std::string, std::map<std::string, std::string>> parse_mule_toml(const std::string &filename)
{
    std::map<std::string, std::map<std::string, std::string>> config;
    std::ifstream file(filename);
    std::string line;
    std::string current_section = "default";

    if (!fs::exists(filename)) {
        std::cerr << "Error: Configuration file '" << filename << "' not found.\n"
                  << "Try running 'mule new <project_name>' first.\n";
        std::exit(1); 
    }

    while (std::getline(file, line))
    {
        //  Basic cleaning
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#')
            continue;

        //  Detect Section [section_name]
        if (line[0] == '[' && line.back() == ']')
        {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        //  Detect Key-Value Pairs
        size_t delim = line.find('=');
        if (delim != std::string::npos)
        {
            std::string key = line.substr(0, delim);
            std::string val = line.substr(delim + 1);

            // Clean key/value whitespace
            key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
            val.erase(std::remove(val.begin(), val.end(), ' '), val.end());
            val.erase(std::remove(val.begin(), val.end(), '\"'), val.end());

            config[current_section][key] = val;
        }
    }
    return config;
}
void run_build()
{
    // Load configuration
    auto config = parse_mule_toml("mule.toml");
    std::string std_version = config["package"]["standard"];
    std::string bin_name = config["package"]["name"];

    std::string compiler = "g++";
    std::vector<std::string> obj_files;
    if (!command_exists("g++")) {
        std::cerr << "Error: 'g++' compiler not found in your PATH.\n";
        return;
    }

    if (!fs::exists("build"))
        fs::create_directory("build");
    std::string include_flags = "-Iinclude ";

    // Automatically include all downloaded dependencies
    fetch_deps();

    // Then use include_flags in your compilation command:
    // std::string cmd = compiler + " " + include_flags + ...
    if (fs::exists(".mule/deps")) {
        for (const auto& entry : fs::directory_iterator(".mule/deps")) {
            if (entry.is_directory()) {
                include_flags += "-I" + entry.path().string() + " ";
                // Many libraries put their headers in an 'include' subfolder
                if (fs::exists(entry.path() / "include")) {
                    include_flags += "-I" + (entry.path() / "include").string() + " ";
                }
            }
        }
    }

    for (const auto &entry : fs::recursive_directory_iterator("src"))
    {
        if (entry.path().extension() == ".cpp")
        {
            fs::path src_path = entry.path();
            fs::path obj_path = fs::path("build") / src_path.filename().replace_extension(".o");

            // Incremental Build Logic
            bool needs_rebuild = !fs::exists(obj_path) ||
                                 fs::last_write_time(src_path) > fs::last_write_time(obj_path);

            if (needs_rebuild)
            {
                // Use the standard from the .toml file
                std::string cmd = compiler + " -std=c++" + std_version + " -c " +
                      src_path.string() + " -o " + obj_path.string() + " " + include_flags;

                std::cout << "Compiling: " << src_path.filename() << std::endl;
                if (std::system(cmd.c_str()) != 0)
                    return;
            }
            obj_files.push_back(obj_path.string());
        }
    }
    std::string link_cmd = compiler + " ";
    for (const auto &obj : obj_files)
        link_cmd += obj + " ";
    link_cmd += "-o build/" + bin_name;

    std::cout << "Linking executable [" << bin_name << "]..." << std::endl;
    std::system(link_cmd.c_str());
}

void run_project()
{
    run_build();

    auto config = parse_mule_toml("mule.toml");
    std::string bin_name = config["package"]["name"]; 
    if(bin_name.empty()) bin_name = "app"; // Fallback
    
    fs::path bin_path = fs::path("build") / bin_name;

    if (fs::exists(bin_path))
    {
        std::cout << "--- Running " << bin_name << " ---" << std::endl;

        std::string run_cmd = "./" + bin_path.string();
        std::system(run_cmd.c_str());
    }
    else
    {
        std::cerr << "Build failed, cannot run." << std::endl;
    }
}

void run_clean()
{
    if (fs::exists("build"))
    {
        fs::remove_all("build");
        std::cout << "Cleaned build artifacts." << std::endl;
    }
}

void fetch_deps()
{
    auto config = parse_mule_toml("mule.toml");

    // Check if dependencies section exists
    if (config.find("dependencies") == config.end())
        return;

    fs::path deps_root = ".mule/deps";
    if (!fs::exists(deps_root))
        fs::create_directories(deps_root);

    for (auto const &[lib_name, url] : config["dependencies"])
    {
        fs::path lib_path = deps_root / lib_name;

        if (!fs::exists(lib_path))
        {
            std::cout << "Downloading dependency: " << lib_name << " from " << url << std::endl;

            std::string cmd = "git clone --depth 1 " + url + " " + lib_path.string();

            if (std::system(cmd.c_str()) != 0)
            {
                std::cerr << "Failed to download " << lib_name << std::endl;
            }
        }
        else
        {
            std::cout << "Dependency " << lib_name << " is up to date." << std::endl;
        }
    }
}

void print_help() {
    std::cout << "Mule: A minimalist C++ build system and package manager\n\n"
              << "Usage: mule <command> [args]\n\n"
              << "Commands:\n"
              << "  new <name>    Create a new C++ project structure\n"
              << "  build         Compile the project based on mule.toml\n"
              << "  run           Build and execute the project binary\n"
              << "  clean         Remove the build directory\n"
              << "  fetch         Download dependencies listed in mule.toml\n"
              << "  --help, -h    Display this help message\n";
}







void cli_main(int argc, char *argv[])
{
   if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        print_help();
        return;
    }

    std::string cmd = argv[1];

    if (cmd == "--version" || cmd == "-v" || cmd == "version") {
        print_version();
        return;
    }

    if (cmd == "new")
    {
        if (argc < 3)
        {
            std::cerr << "Error: 'new' requires a project name." << std::endl;
            return;
        }
        create_template(argv[2]);
    }
    else if (cmd == "build")
    {
        run_build();
    }
    else if (cmd == "run")
    {
        run_project();
    }
    else if (cmd == "clean")
    {
        run_clean();
    }
}