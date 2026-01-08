#include "../../include/core/PackageManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace mule {

    std::string get_git_head(const fs::path& repo_path) {
        std::string cwd = fs::current_path().string();
        fs::current_path(repo_path);
        std::system("git rev-parse HEAD > .git_head_tmp");
        std::ifstream f(".git_head_tmp");
        std::string hash;
        f >> hash;
        fs::remove(".git_head_tmp");
        fs::current_path(cwd);
        return hash;
    }

    std::vector<Dependency> PackageManager::fetch_dependencies(const std::vector<Dependency>& deps) {
        std::vector<Dependency> resolved;
        if (deps.empty()) return resolved;

        fs::path deps_root = ".mule/deps";
        if (!fs::exists(deps_root))
            fs::create_directories(deps_root);

        for (const auto& dep : deps) {
            Dependency res = dep;
            fs::path lib_path = deps_root / dep.name;

            // 1. Handle Path Dependencies
            if (!dep.path.empty()) {
                if (fs::exists(lib_path)) fs::remove(lib_path);
                fs::path target = fs::absolute(dep.path);
                if (!fs::exists(target)) {
                    std::cerr << "Error: Local dependency '" << dep.name << "' not found at " << target << std::endl;
                    continue; // Should probably error out hard here
                }
                try {
                    fs::create_directory_symlink(target, lib_path);
                    std::cout << "Linked local dependency: " << dep.name << " -> " << target.string() << std::endl;
                } catch (...) {
                    fs::copy(target, lib_path, fs::copy_options::recursive);
                }
                res.path = target.string();
                resolved.push_back(res);
                continue;
            }

            // 2. Handle Git Dependencies
            if (!dep.git.empty()) {
                if (!fs::exists(lib_path)) {
                    std::cout << "Downloading dependency: " << dep.name << " from " << dep.git << std::endl;
                    std::string cmd = "git clone " + dep.git + " " + lib_path.string();
                    if (std::system(cmd.c_str()) != 0) {
                        std::cerr << "Failed to download " << dep.name << std::endl;
                        continue;
                    }
                }

                if (!dep.tag.empty() || !dep.commit.empty()) {
                    std::string target = dep.tag.empty() ? dep.commit : dep.tag;
                    std::string cwd = fs::current_path().string();
                    fs::current_path(lib_path);
                    std::system(("git checkout " + target + " > /dev/null 2>&1").c_str());
                    fs::current_path(cwd);
                }
                
                // Always get the resolved commit hash
                res.commit = get_git_head(lib_path);
                resolved.push_back(res);
            }
        }
        return resolved;
    }

    void PackageManager::write_lockfile(const std::vector<Dependency>& resolved) {
        std::ofstream lock("mule.lock");
        lock << "# Verified dependency snapshots\n\n";
        
        lock << "[dependencies]\n";
        for (const auto& dep : resolved) {
            lock << dep.name << " = { ";
            if (!dep.path.empty()) {
                lock << "path = \"" << dep.path << "\"";
            } else {
                lock << "git = \"" << dep.git << "\", commit = \"" << dep.commit << "\"";
                if (!dep.tag.empty()) lock << ", tag = \"" << dep.tag << "\"";
            }
            lock << " }\n";
        }
        lock.close();
    }
}
