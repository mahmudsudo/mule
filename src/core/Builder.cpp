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

    static std::string get_lib_extension(const std::string& type) {
        if (type == "static-lib") {
            #ifdef _WIN32
                return ".lib";
            #else
                return ".a";
            #endif
        } else if (type == "shared-lib") {
            #ifdef _WIN32
                return ".dll";
            #else
                return ".so";
            #endif
        }
        return get_exe_extension();
    }

    static std::string make_compile_cmd(CompilerType type, const std::string& compiler, const std::string& src, const std::string& obj, const std::string& std_ver, const std::string& flags, bool is_shared) {
        std::string pic = "";
        if (is_shared && type != CompilerType::MSVC) pic = "-fPIC ";

        if (type == CompilerType::MSVC) {
            return compiler + " /std:c++" + std_ver + " /c " + src + " /Fo" + obj + " /EHsc " + flags;
        } else {
            return compiler + " -std=c++" + std_ver + " " + pic + "-c " + src + " -o " + obj + " " + flags;
        }
    }

    static std::string make_link_cmd(CompilerType type, const std::string& compiler, const std::vector<std::string>& objs, const std::string& bin_name, const Config& config) {
        std::string cmd = compiler + " ";
        if (config.type == "shared-lib" && type != CompilerType::MSVC) cmd += "-shared ";
        
        for (const auto& obj : objs) cmd += obj + " ";
        
        std::string ext = get_lib_extension(config.type);
        std::string prefix = (config.type != "bin" && type != CompilerType::MSVC) ? "lib" : "";
        std::string output = "build/" + prefix + bin_name + ext;

        if (type == CompilerType::MSVC) {
            if (config.type == "shared-lib") cmd += "/LD ";
            cmd += "/Fe" + output + " ";
        } else {
            cmd += "-o " + output + " ";
        }

        // Add build flags
        for (const auto& dir : config.build.lib_dirs) {
            cmd += (type == CompilerType::MSVC ? "/LIBPATH:" : "-L") + dir + " ";
        }
        for (const auto& lib : config.build.libs) {
            cmd += (type == CompilerType::MSVC ? "" : "-l") + lib + (type == CompilerType::MSVC ? ".lib " : " ");
        }
        for (const auto& flag : config.build.flags) {
            cmd += flag + " ";
        }

        return cmd;
    }

    static std::string make_archive_cmd(CompilerType type, const std::vector<std::string>& objs, const std::string& bin_name) {
        std::string ext = get_lib_extension("static-lib");
        std::string prefix = (type != CompilerType::MSVC) ? "lib" : "";
        std::string output = "build/" + prefix + bin_name + ext;

        if (type == CompilerType::MSVC) {
            std::string cmd = "lib /OUT:" + output + " ";
            for (const auto& obj : objs) cmd += obj + " ";
            return cmd;
        } else {
            std::string cmd = "ar rcs " + output + " ";
            for (const auto& obj : objs) cmd += obj + " ";
            return cmd;
        }
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

        // Add custom include directories
        for (const auto& dir : config.build.include_dirs) {
            include_flags += (compiler_type == CompilerType::MSVC ? "/I" : "-I") + dir + " ";
        }

        // Add custom flags to include_flags for compilation
        for (const auto& flag : config.build.flags) {
            include_flags += flag + " ";
        }

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
                        std::string cmd = make_compile_cmd(compiler_type, compiler_cmd, src_path.string(), obj_path.string(), config.standard, include_flags, config.type == "shared-lib");

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
        
        std::string link_cmd;
        if (config.type == "static-lib") {
            link_cmd = make_archive_cmd(compiler_type, obj_files, config.project_name);
            std::cout << "Archiving static library [lib" << config.project_name << "]..." << std::endl;
        } else {
            link_cmd = make_link_cmd(compiler_type, compiler_cmd, obj_files, config.project_name, config);
            std::string target_type = (config.type == "shared-lib") ? "shared library" : "executable";
            std::cout << "Linking " << target_type << " [" << config.project_name << "]..." << std::endl;
        }

        if (std::system(link_cmd.c_str()) != 0) {
             std::cerr << "Linking/Archiving failed." << std::endl;
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
