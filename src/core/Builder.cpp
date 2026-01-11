#include "../../include/core/Builder.h"
#include "../../include/core/Utils.h"
#include "../../include/core/PackageManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace mule {

    // Helper functions (internal linkage)
    static std::string get_obj_extension(CompilerType type) {
        return (type == CompilerType::MSVC) ? ".obj" : ".o";
    }

    static std::string get_exe_extension() {
        return get_exe_ext();
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
        for (const auto& lflag : config.build.linker_flags) {
            cmd += lflag + " ";
        }

        // Add default suppression for common Linux linker noise if using GNU ld (common on Linux)
        #ifndef _WIN32
        if (type != CompilerType::MSVC) {
            // Suppress .sframe and other mismatch noise if possible
            // Note: we use -Wl to pass these to the linker
            // cmd += "-Wl,--no-warn-search-mismatch "; // Some compilers might complain if ld doesn't support it
        }
        #endif

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


    static std::string replace_all(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return str;
    }

    static bool file_contains(const std::string& path, const std::string& search) {
        if (search.empty()) return true;
        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line)) {
            if (line.find(search) != std::string::npos) return true;
        }
        return false;
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

    static std::string exec(const char* cmd) {
        return exec_cmd(cmd);
    }

    void Builder::build(const Config& config) {
        std::string compiler_cmd;
        CompilerType compiler_type = detect_compiler(compiler_cmd);

        if (compiler_type == CompilerType::Unknown) {
            std::cerr << "Error: No suitable compiler (g++, clang++, cl) found in PATH.\n";
            return;
        }

        if (!fs::exists("build"))
            fs::create_directories("build");
        
        if (!fs::exists("build/generated"))
            fs::create_directories("build/generated");

        Config active_config = config;

        // Qt Support: Add include paths and libraries
        if (active_config.qt.enabled) {
            for (const auto& module : active_config.qt.modules) {
                std::string pkg = "Qt5" + module; // Assuming Qt5 for now, can be improved
                std::string cflags = exec(("pkg-config --cflags " + pkg).c_str());
                std::string libs = exec(("pkg-config --libs " + pkg).c_str());

                if (cflags.find("not found") != std::string::npos || cflags.empty()) {
                    // Try Qt6 if Qt5 fails
                    pkg = "Qt6" + module;
                    cflags = exec(("pkg-config --cflags " + pkg).c_str());
                    libs = exec(("pkg-config --libs " + pkg).c_str());
                }

                if (!cflags.empty()) {
                    // Split flags and add to config
                    size_t pos = 0;
                    while ((pos = cflags.find("-I")) != std::string::npos) {
                        cflags.erase(0, pos + 2);
                        size_t end = cflags.find(' ');
                        active_config.build.include_dirs.push_back(cflags.substr(0, end));
                        if (end == std::string::npos) break;
                        cflags.erase(0, end);
                    }
                    // Simple hack for other flags
                    active_config.build.flags.push_back(cflags);
                }
                if (!libs.empty()) {
                    active_config.build.linker_flags.push_back(libs); // Linker flags
                }
            }
        }

        // Run Generators
        std::vector<std::string> generated_sources;
        for (const auto& gen : active_config.generators) {
            if (!fs::exists("src")) continue;
            for (const auto& entry : fs::recursive_directory_iterator("src")) {
                if (entry.path().extension().string() == gen.input_extension) {
                    if (!file_contains(entry.path().string(), gen.match_content)) {
                        continue;
                    }

                    std::string input = entry.path().string();
                    std::string output = "build/generated/" + entry.path().filename().string() + gen.output_extension;
                    
                    bool needs_gen = !fs::exists(output) || fs::last_write_time(input) > fs::last_write_time(output);
                    
                    if (needs_gen) {
                        std::string cmd = gen.command;
                        cmd = replace_all(cmd, "{input}", input);
                        cmd = replace_all(cmd, "{output}", output);
                        
                        std::cout << "Generating: " << output << " from " << entry.path().filename() << " (" << gen.name << ")" << std::endl;
                        if (std::system(cmd.c_str()) != 0) {
                            std::cerr << "Generator " << gen.name << " failed for " << input << std::endl;
                            return;
                        }
                    }
                    
                    if (fs::path(output).extension() == ".cpp" || fs::path(output).extension() == ".cc") {
                        generated_sources.push_back(output);
                    }
                }
            }
        }
        
        std::string include_flags;
        if (compiler_type == CompilerType::MSVC)
            include_flags = "/Iinclude /Ibuild/generated ";
        else
            include_flags = "-Iinclude -Ibuild/generated ";

        // Add custom include directories
        for (const auto& dir : active_config.build.include_dirs) {
            if (dir.empty()) continue;
            include_flags += (compiler_type == CompilerType::MSVC ? "/I" : "-I") + dir + " ";
        }

        // Add custom flags to include_flags for compilation
        for (const auto& flag : active_config.build.flags) {
            if (flag.empty()) continue;
            include_flags += flag + " ";
        }

        // Add defines
        for (const auto& def : active_config.build.defines) {
            if (def.empty()) continue;
            include_flags += (compiler_type == CompilerType::MSVC ? "/D" : "-D") + def + " ";
        }

        // Fetch dependencies using PackageManager
        auto resolved = PackageManager::fetch_dependencies(active_config.dependencies);
        PackageManager::write_lockfile(resolved);
        PackageManager::build_dependencies(resolved, compiler_type);

        if (fs::exists(".mule/deps")) {
            for (const auto& entry : fs::directory_iterator(".mule/deps")) {
                if (entry.is_directory()) {
                    // Try to discover include directories automatically
                    std::vector<std::string> search_paths = {
                        entry.path().string(),
                        (entry.path() / "include").string(),
                        (entry.path() / "src").string()
                    };

                    for (const auto& sp : search_paths) {
                        if (fs::exists(sp)) {
                            include_flags += (compiler_type == CompilerType::MSVC ? "/I" : "-I") + sp + " ";
                        }
                    }

                    // Also search for libraries in 'build' or 'lib' directories of the dependency
                    std::vector<std::string> lib_paths = {
                        (entry.path() / "build").string(),
                        (entry.path() / "lib").string()
                    };
                    for (const auto& lp : lib_paths) {
                        if (fs::exists(lp)) {
                            active_config.build.lib_dirs.push_back(lp);
                        }
                    }
                }
            }
        }

        std::vector<std::string> obj_files;
        std::string obj_ext = get_obj_extension(compiler_type);

        // Compile regular sources
        if (fs::exists("src")) {
            for (const auto &entry : fs::recursive_directory_iterator("src")) {
                if (entry.path().extension() == ".cpp") {
                    fs::path src_path = entry.path();
                    fs::path obj_path = fs::path("build") / src_path.filename().replace_extension(obj_ext);

                    bool needs_rebuild = !fs::exists(obj_path) ||
                                         fs::last_write_time(src_path) > fs::last_write_time(obj_path);

                    if (needs_rebuild) {
                        std::string cmd = make_compile_cmd(compiler_type, compiler_cmd, src_path.string(), obj_path.string(), active_config.standard, include_flags, active_config.type == "shared-lib");

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

        // Compile generated sources
        for (const auto& src : generated_sources) {
            fs::path src_path = src;
            fs::path obj_path = fs::path("build") / src_path.filename().replace_extension(obj_ext);

            bool needs_rebuild = !fs::exists(obj_path) ||
                                    fs::last_write_time(src_path) > fs::last_write_time(obj_path);

            if (needs_rebuild) {
                std::string cmd = make_compile_cmd(compiler_type, compiler_cmd, src_path.string(), obj_path.string(), active_config.standard, include_flags, active_config.type == "shared-lib");

                std::cout << "Compiling generated: " << src_path.filename() << std::endl;
                if (std::system(cmd.c_str()) != 0) {
                    std::cerr << "Compilation failed for generated source " << src_path << std::endl;
                    return;
                }
            }
            obj_files.push_back(obj_path.string());
        }
        
        std::string link_cmd;
        if (active_config.type == "static-lib") {
            link_cmd = make_archive_cmd(compiler_type, obj_files, active_config.project_name);
            std::cout << "Archiving static library [lib" << active_config.project_name << "]..." << std::endl;
        } else {
            link_cmd = make_link_cmd(compiler_type, compiler_cmd, obj_files, active_config.project_name, active_config);
            std::string target_type = (active_config.type == "shared-lib") ? "shared library" : "executable";
            std::cout << "Linking " << target_type << " [" << active_config.project_name << "]..." << std::endl;
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
