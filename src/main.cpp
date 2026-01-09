#include <iostream>
#include <string>
#include <vector>
#include "../include/core/Project.h"
#include "../include/core/Builder.h"
#include "../include/core/ConfigParser.h"
#include "../include/core/Utils.h"
#include "../include/core/PackageManager.h"
#include "../include/core/TestRunner.h"

void print_help() {
    std::cout << "Mule: A minimalist C++ build system and package manager\n\n"
              << "Usage: mule <command> [args]\n\n"
              << "Commands:\n"
              << "  new <name> [--lib]  Create a new project or library structure\n"
              << "  build               Compile the project based on mule.toml\n"
              << "  run                 Build and execute the project binary\n"
              << "  clean               Remove the build directory\n"
              << "  fetch               Download dependencies listed in mule.toml\n"
              << "  test                Run tests found in tests/ (integration style)\n"
              << "  --help, -h          Display this help message\n"
              << "  --version, -v       Display version information\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        print_help();
        return 0;
    }

    if (cmd == "--version" || cmd == "-v" || cmd == "version") {
        mule::print_version();
        return 0;
    }

    if (cmd == "new") {
        if (argc < 3) {
            std::cerr << "Error: 'new' requires a project name." << std::endl;
            return 1;
        }
        bool is_lib = false;
        if (argc >= 4 && std::string(argv[3]) == "--lib") {
            is_lib = true;
        }
        mule::Project::create(argv[2], is_lib);
        return 0;
    }

    // Commands that require a config
    mule::Config config;
    try {
        if (cmd == "build" || cmd == "run" || cmd == "fetch" || cmd == "test") {
            config = mule::ConfigParser::parse("mule.toml");
        }
    } catch (...) {
        // Config parser handles its own caching/error reporting mostly, but good to be safe
        return 1;
    }

    if (cmd == "build") {
        mule::Builder::build(config);
    } else if (cmd == "run") {
        mule::Builder::run(config);
    } else if (cmd == "clean") {
        mule::Builder::clean();
    } else if (cmd == "fetch") {
        auto resolved = mule::PackageManager::fetch_dependencies(config.dependencies);
        mule::PackageManager::write_lockfile(resolved);
    } else if (cmd == "test") {
        mule::TestRunner::run_tests(config);
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        print_help();
        return 1;
    }

    return 0;
}
