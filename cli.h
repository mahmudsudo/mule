#ifndef CLI_H
#define CLI_H

#include <string>
#include <map>
#include <vector>

// Forward declarations: telling the compiler these exist
enum class CompilerType { GCC, Clang, MSVC, Unknown };

CompilerType detect_compiler(std::string& out_cmd);
std::string get_obj_extension(CompilerType type);
std::string make_compile_cmd(CompilerType type, const std::string& compiler, const std::string& src, const std::string& obj, const std::string& std_ver, const std::string& flags);
std::string make_link_cmd(CompilerType type, const std::string& compiler, const std::vector<std::string>& objs, const std::string& bin_name);

void run_build();
void run_project();
void run_clean();
void fetch_deps();
void create_template(const std::string &project_name);
// CLI Commands
void cli_main(int argc, char *argv[]);
std::map<std::string, std::map<std::string, std::string>> parse_mule_toml(const std::string &filename);

#endif