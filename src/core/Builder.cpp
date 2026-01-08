#include "../../include/core/Builder.h"
#include "../../include/core/Utils.h"
#include "../../include/core/PackageManager.h"
#include <iostream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace mule {

    // Helper functions (internal linkage)
    static std::string get_obj_extension(CompilerType type) {
        return (type == CompilerType::MSVC) ? ".obj" : ".o";
    }

    static std::string get_exe_extension() {
    #ifdef _WIN32
        return ".exe";
    #else
        return "";
    #endif
    }

    static std::string make_compile_cmd(CompilerType type, const std::string& compiler, const std::string& src, const std::string& obj, const std::string& std_ver, const std::string& flags) {
        if (type == CompilerType::MSVC) {
            return compiler + " /std:c++" + std_ver + " /c " + src + " /Fo" + obj + " /EHsc " + flags;
        } else {
            return compiler + " -std=c++" + std_ver + " -c " + src + " -o " + obj + " " + flags;
        }
    }

    static std::string make_link_cmd(CompilerType type, const std::string& compiler, const std::vector<std::string>& objs, const std::string& bin_name) {
        std::string cmd = compiler + " ";
        for (const auto& obj : objs) cmd += obj + " ";
        
        std::string final_bin_name = bin_name + get_exe_extension();

        if (type == CompilerType::MSVC) {
            cmd += "/Fe" + std::string("build/") + final_bin_name;
        } else {
            cmd += "-o build/" + final_bin_name;
        }
        return cmd;
    }


    CompilerType Builder::detect_compiler(std::string& out_cmd) {
        if (command_exists("clang++")) {
            out_cmd = "clang++";
            return CompilerType::Clang;
        }
        if (command_exists("g++")) {
            out_cmd = "g++";
            return CompilerType::GCC;
        }
        if (command_exists("cl")) {
            out_cmd = "cl";
            return CompilerType::MSVC;
        }
        return CompilerType::Unknown;
    }

    void Builder::build(const Config& config) {
        std::string compiler_cmd;
        CompilerType compiler_type = detect_compiler(compiler_cmd);

        if (compiler_type == CompilerType::Unknown) {
            std::cerr << "Error: No suitable compiler (g++, clang++, cl) found in PATH.\n";
            return;
        }

        if (!fs::exists("build"))
            fs::create_directory("build");
        
        std::string include_flags;
        if (compiler_type == CompilerType::MSVC)
            include_flags = "/Iinclude ";
        else
            include_flags = "-Iinclude ";

        // Fetch dependencies using PackageManager
        auto resolved = PackageManager::fetch_dependencies(config.dependencies);
        PackageManager::write_lockfile(resolved);

        if (fs::exists(".mule/deps")) {
            for (const auto& entry : fs::directory_iterator(".mule/deps")) {
                if (entry.is_directory()) {
                    if (compiler_type == CompilerType::MSVC)
                        include_flags += "/I" + entry.path().string() + " ";
                    else
                        include_flags += "-I" + entry.path().string() + " ";
                    
                    if (fs::exists(entry.path() / "include")) {
                        if (compiler_type == CompilerType::MSVC)
                            include_flags += "/I" + (entry.path() / "include").string() + " ";
                        else
                            include_flags += "-I" + (entry.path() / "include").string() + " ";
                    }
                }
            }
        }

        std::vector<std::string> obj_files;
        std::string obj_ext = get_obj_extension(compiler_type);

        if (fs::exists("src")) {
            for (const auto &entry : fs::recursive_directory_iterator("src")) {
                if (entry.path().extension() == ".cpp") {
                    fs::path src_path = entry.path();
                    fs::path obj_path = fs::path("build") / src_path.filename().replace_extension(obj_ext);

                    bool needs_rebuild = !fs::exists(obj_path) ||
                                         fs::last_write_time(src_path) > fs::last_write_time(obj_path);

                    if (needs_rebuild) {
                        std::string cmd = make_compile_cmd(compiler_type, compiler_cmd, src_path.string(), obj_path.string(), config.standard, include_flags);

                        std::cout << "Compiling: " << src_path.filename() << std::endl;
                        if (std::system(cmd.c_str()) != 0) {
                            std::cerr << "Compilation failed for " << src_path << std::endl;
                            return;
                        }
                    }
                    obj_files.push_back(obj_path.string());
                }
            }
        }
        
        std::string link_cmd = make_link_cmd(compiler_type, compiler_cmd, obj_files, config.project_name);

        std::cout << "Linking executable [" << config.project_name << "]..." << std::endl;
        if (std::system(link_cmd.c_str()) != 0) {
             std::cerr << "Linking failed." << std::endl;
        }
    }

    void Builder::run(const Config& config) {
        // Ensure build
        build(config);

        std::string bin_name = config.project_name;
        if(bin_name.empty()) bin_name = "app"; 
        
        std::string exe_name = bin_name + get_exe_extension();
        fs::path bin_path = fs::path("build") / exe_name;

        if (fs::exists(bin_path)) {
            std::cout << "--- Running " << bin_name << " ---" << std::endl;

            std::string run_cmd = "./" + bin_path.string(); 
    #ifdef _WIN32
            run_cmd = bin_path.string();
    #endif
            std::system(run_cmd.c_str());
        } else {
            std::cerr << "Build failed, cannot run." << std::endl;
        }
    }

    void Builder::clean() {
        if (fs::exists("build")) {
            fs::remove_all("build");
            std::cout << "Cleaned build artifacts." << std::endl;
        }
    }
}
