#include "../../include/core/Project.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace mule {

    void Project::create(const std::string& project_name, bool is_lib) {
        try {
            fs::path project_path = project_name;
            fs::create_directories(project_path / "src");
            fs::create_directories(project_path / "include");

            if (is_lib) {
                std::ofstream lib_file(project_path / "src" / "lib.cpp");
                lib_file << "#include <iostream>\n\n"
                         << "void hello() {\n"
                         << "    std::cout << \"Hello from library " << project_name << "!\" << std::endl;\n"
                         << "}\n";
                lib_file.close();

                std::ofstream header_file(project_path / "include" / (project_name + ".h"));
                header_file << "#pragma once\n\n"
                            << "void hello();\n";
                header_file.close();
            } else {
                std::ofstream main_file(project_path / "src" / "main.cpp");
                main_file << "#include <iostream>\n\n"
                          << "int main() {\n"
                          << "    std::cout << \"Hello from " << project_name << "!\" << std::endl;\n"
                          << "    return 0;\n"
                          << "}\n";
                main_file.close();
            }

            std::ofstream config_file(project_path / "mule.toml");
            config_file << "[package]\n"
                        << "name = \"" << project_name << "\"\n"
                        << "version = \"0.1.0\"\n"
                        << "standard = \"17\"\n";
            if (is_lib) {
                config_file << "type = \"static-lib\"\n";
            }
            config_file.close();

            std::cout << "Successfully created " << (is_lib ? "library" : "project") << ": " << project_name << std::endl;
        }
        catch (const std::exception &e) {
            std::cerr << "Scaffolding failed: " << e.what() << std::endl;
        }
    }
}
