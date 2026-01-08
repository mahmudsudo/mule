#include "../../include/core/TestRunner.h"
#include "../../include/core/Builder.h" // Reuse compiler detection
#include "../../include/core/Utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace mule {

    // Duplicate helper since I didn't verify if I should expose it publicly in Builder.h yet
    // In a real refactor, move compiler utilities to a separate module.
    static std::string get_exe_ext() {
    #ifdef _WIN32
        return ".exe";
    #else
        return "";
    #endif
    }

    void TestRunner::run_tests(const Config& config) {
        // 1. Look for test files: "tests/*.cpp" or "*_test.cpp"
        std::vector<std::string> test_sources;
        
        if (fs::exists("tests")) {
            for (const auto& entry : fs::recursive_directory_iterator("tests")) {
                if (entry.path().extension() == ".cpp") {
                    test_sources.push_back(entry.path().string());
                }
            }
        }
        
        // Also look in src for *_test.cpp? Maybe stick to tests/ folder for purity first.
        // Let's stick to 'tests/' directory convention for now to be like Cargo's integration tests.

        if (test_sources.empty()) {
            std::cout << "No tests found in 'tests/' directory." << std::endl;
            return;
        }

        // 2. We need to compile the library sources (excluding main.cpp) + test sources + test harness
        // Identifying main.cpp is tricky if not standardized. We will assume src/main.cpp is the app entry and skip it.
        
        std::vector<std::string> lib_sources;
        if (fs::exists("src")) {
            for (const auto& entry : fs::recursive_directory_iterator("src")) {
                if (entry.path().extension() == ".cpp") {
                    if (entry.path().filename() != "main.cpp") {
                        lib_sources.push_back(entry.path().string());
                    }
                }
            }
        }

        // 3. Generate a temporary main file for the test runner? 
        // Or we can rely on the fact that we can link against a main provided by us?
        // Let's generate a test_main.cpp in build/
        
        if (!fs::exists("build")) fs::create_directory("build");
        
        std::string test_main_path = "build/mule_test_main.cpp";
        std::ofstream test_main(test_main_path);
        test_main << "#include \"../include/mule_test.h\"\n"
                  << "#include <iostream>\n"
                  << "int main() {\n"
                  << "    int passed = 0;\n"
                  << "    int failed = 0;\n"
                  << "    for (const auto& test : mule::get_tests()) {\n"
                  << "        try {\n"
                  << "            test.func();\n"
                  << "            std::cout << \"[PASS] \" << test.name << std::endl;\n"
                  << "            passed++;\n"
                  << "        } catch (const std::exception& e) {\n"
                  << "            std::cout << \"[FAIL] \" << test.name << \": \" << e.what() << std::endl;\n"
                  << "            failed++;\n"
                  << "        }\n"
                  << "    }\n"
                  << "    std::cout << \"Result: \" << passed << \" passed, \" << failed << \" failed.\" << std::endl;\n"
                  << "    return failed > 0 ? 1 : 0;\n"
                  << "}\n";
        test_main.close();

        // 4. Compile everything
        // Note: This duplicates logic from Builder. Use a better abstraction later.
        std::string compiler = "g++"; // default
        if (mule::command_exists("clang++")) compiler = "clang++";
        // MSVC support omitted for brevity in this step, need to copy Builder logic

        std::string cmd = compiler + " -std=c++" + config.standard + " -Iinclude -I. ";
        
        // Add all sources
        cmd += test_main_path + " ";
        for (const auto& s : test_sources) cmd += s + " ";
        for (const auto& s : lib_sources) cmd += s + " ";

        std::string output_bin = "build/test_runner" + get_exe_ext();
        cmd += "-o " + output_bin;

        std::cout << "Compiling tests..." << std::endl;
        if (std::system(cmd.c_str()) != 0) {
            std::cerr << "Test compilation failed." << std::endl;
            return;
        }

        // 5. Run tests
        std::cout << "Running tests..." << std::endl;
        std::system(("./" + output_bin).c_str());
    }
}
