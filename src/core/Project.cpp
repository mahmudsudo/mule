#include "../../include/core/Project.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace mule {

    void Project::create(const std::string& project_name) {
        try {
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
        catch (const std::exception &e) {
            std::cerr << "Scaffolding failed: " << e.what() << std::endl;
        }
    }
}
