#include "../../include/core/TestRunner.h"
#include "../../include/core/Builder.h" // Reuse compiler detection
#include "../../include/core/Utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace mule {



    void TestRunner::run_tests(const Config& config) {
        std::string compiler_cmd;
        CompilerType compiler_type = Builder::detect_compiler(compiler_cmd);

        if (compiler_type == CompilerType::Unknown) {
            std::cerr << "Error: No suitable compiler found for tests.\n";
            return;
        }

        if (!fs::exists("build")) fs::create_directory("build");

        // 1. Collect all library sources (src/*.cpp excluding main.cpp and *_test.cpp)
        std::vector<std::string> lib_sources;
        std::vector<std::string> unit_test_sources;
        if (fs::exists("src")) {
            for (const auto& entry : fs::recursive_directory_iterator("src")) {
                if (entry.path().extension() == ".cpp") {
                    std::string filename = entry.path().filename().string();
                    if (filename == "main.cpp") continue;
                    
                    if (filename.find("_test.cpp") != std::string::npos) {
                        unit_test_sources.push_back(entry.path().string());
                    } else {
                        lib_sources.push_back(entry.path().string());
                    }
                }
            }
        }

        // 2. Integration Tests (tests/*.cpp)
        std::vector<std::string> integration_test_sources;
        if (fs::exists("tests")) {
            for (const auto& entry : fs::directory_iterator("tests")) {
                if (entry.path().extension() == ".cpp") {
                    integration_test_sources.push_back(entry.path().string());
                }
            }
        }

        if (unit_test_sources.empty() && integration_test_sources.empty()) {
            std::cout << "No tests found." << std::endl;
            return;
        }

        // Common include flags (defines, dependencies, standard)
        // We really should extract this from Builder::build but for now we'll reconstruct
        std::string include_flags = "-Iinclude -I. ";
        for (const auto& dir : config.build.include_dirs) include_flags += "-I" + dir + " ";
        for (const auto& def : config.build.defines) include_flags += "-D" + def + " ";
        // Discovery for dependencies
        if (fs::exists(".mule/deps")) {
            for (const auto& entry : fs::directory_iterator(".mule/deps")) {
                if (entry.is_directory()) {
                    include_flags += "-I" + entry.path().string() + " ";
                    if (fs::exists(entry.path() / "include")) include_flags += "-I" + (entry.path() / "include").string() + " ";
                }
            }
        }

        int total_passed = 0;
        int total_failed = 0;

        // --- UNIT TESTS ---
        if (!unit_test_sources.empty()) {
            std::cout << "\033[1;36mRunning unit tests...\033[0m" << std::endl;
            
            std::string test_main_path = "build/unit_test_main.cpp";
            std::ofstream test_main(test_main_path);
            test_main << "#include \"include/mule_test.h\"\n"
                      << "#include <iostream>\n"
                      << "int main() {\n"
                      << "    int passed = 0; int failed = 0;\n"
                      << "    for (const auto& test : mule::get_tests()) {\n"
                      << "        try { test.func(); std::cout << \"  [PASS] \" << test.name << std::endl; passed++; }\n"
                      << "        catch (const std::exception& e) { std::cout << \"  [FAIL] \" << test.name << \": \" << e.what() << std::endl; failed++; }\n"
                      << "    }\n"
                      << "    return failed;\n"
                      << "}\n";
            test_main.close();

            std::string output_bin = "build/unit_tests" + get_exe_ext();
            std::string cmd = compiler_cmd + " -std=c++" + config.standard + " " + include_flags + " " + test_main_path + " ";
            for (const auto& s : lib_sources) cmd += s + " ";
            for (const auto& s : unit_test_sources) cmd += s + " ";
            cmd += "-o " + output_bin;

            if (std::system(cmd.c_str()) == 0) {
                int res = std::system(("./" + output_bin).c_str());
                if (res != 0) total_failed++; else total_passed++;
            } else {
                std::cerr << "Unit test compilation failed." << std::endl;
                total_failed++;
            }
        }

        // --- INTEGRATION TESTS ---
        for (const auto& test_src : integration_test_sources) {
            std::string test_name = fs::path(test_src).stem().string();
            std::cout << "\033[1;36mRunning integration test: " << test_name << "...\033[0m" << std::endl;

            // For integration tests, we need a main if not present, but usually integration tests have their own main
            // However, to keep it consistent with MULE_TEST macro, we generate a small wrapper if needed or just compile as is.
            // Cargo's integration tests ARE standalone bins.
            
            std::string output_bin = "build/test_" + test_name + get_exe_ext();
            std::string cmd = compiler_cmd + " -std=c++" + config.standard + " " + include_flags + " " + test_src + " ";
            for (const auto& s : lib_sources) cmd += s + " ";
            cmd += "-o " + output_bin;

            if (std::system(cmd.c_str()) == 0) {
                int res = std::system(("./" + output_bin).c_str());
                if (res != 0) total_failed++; else total_passed++;
            } else {
                std::cerr << "Integration test " << test_name << " compilation failed." << std::endl;
                total_failed++;
            }
        }

        std::cout << "\n\033[1;32mTest Summary: " << total_passed << " passed, " << total_failed << " failed.\033[0m" << std::endl;
    }
}
