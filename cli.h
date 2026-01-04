#ifndef CLI_H
#define CLI_H

#include <string>
#include <map>
#include <vector>

// Forward declarations: telling the compiler these exist
void run_build();
void run_project();
void run_clean();
void fetch_deps();
void create_template(const std::string &project_name);
// CLI Commands
void cli_main(int argc, char *argv[]);
std::map<std::string, std::map<std::string, std::string>> parse_mule_toml(const std::string &filename);

#endif