##  Mule: A Minimalist C++ Build System & Package Manager

Mule is a lightweight, convention-over-configuration build tool for C++ developers. Inspired by Rust's Cargo, Mule simplifies project scaffolding, handles incremental builds, and manages dependencies directly from Git repositories or local paths without the complexity of CMake.

### 🚀 Key Features

* **Zero Configuration:** Start a project in seconds.
* **Incremental Builds:** Only recompiles files that have changed, saving you time.
* **Advanced Dependency Management:** Supports Git repositories (with commit/tag/branch locking) and local path dependencies.
* **Integrated Testing Framework:** Built-in `mule test` command with a lightweight header-only assertion library.
* **Library & Executable Support:** Easily create static libraries, shared libraries, or executables.
* **Native TOML Configuration:** Manage your project settings in a human-readable `mule.toml` file.
* **Compiler Agnostic:** Automatically detects and uses GCC, Clang, or MSVC on your system.
* **Cross-Platform Ready:** Works seamlessly on Linux, macOS, and Windows.

---

## 🛠 Installation

### Prerequisites

* A C++17 compatible compiler (`g++`, `clang++`, or `cl.exe`).
* `git` (for dependency management).

### Binary Installation (Quickest)
If you don't want to build from source, download the latest pre-compiled binary:

1. Download the `mule` binary from the Releases page.
2. Make it executable and move it to your path:

```bash
chmod +x mule
sudo mv mule /usr/local/bin/
```

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/mahmudsudo/mule.git
cd mule
```

2. Compile the binary (choose your compiler):

**Using GCC:**
```bash
g++ -std=c++17 src/main.cpp src/core/*.cpp -o mule
```

**Using Clang:**
```bash
clang++ -std=c++17 src/main.cpp src/core/*.cpp -o mule
```

**Using MSVC (Windows):**
Open a Developer Command Prompt for VS and run:
```cmd
cl /std:c++17 /EHsc src/main.cpp src/core/*.cpp /Fe:mule.exe
```

3. Move the binary to your `PATH`.

---

## 🚀 Examples

Check out the [examples](./examples) directory to see Mule in action with:

* Simple Hello World
* Static Libraries
* Qt Integration
* Custom Code Generators

---

## 📖 Usage Guide

### 1. Create a New Project

Generate a standard executable project:
```bash
mule new my_project
```

Or a library project:
```bash
mule new my_lib --lib
```

### 2. Dependency Management

Configure your dependencies in `mule.toml`.

#### Git Dependencies
You can point to a Git repository and optionally specify a `tag`, `commit`, or `branch`.
```toml
[dependencies]
fmt = "https://github.com/fmtlib/fmt.git" # Latest master
json = { git = "https://github.com/nlohmann/json.git", commit = "bc889af" }
range-v3 = { git = "https://github.com/ericniebler/range-v3.git", tag = "0.12.0" }
```

#### Local Path Dependencies
Useful for monorepos or local development.
```toml
[dependencies]
my_utils = { path = "../my_utils" }
```

#### Fetching and Locking
Run `mule fetch` to download dependencies. This generates a `mule.lock` file, pinning the exact commit hashes for reproducible builds.

### 3. Build Configuration

Control how your project is built with the `[build]` and `[package]` sections.

```toml
[package]
name = "my_app"
standard = "20"
type = "bin" # Options: "bin" (default), "static-lib", "shared-lib"

[build]
include_dirs = ["include", "third_party/include"]
lib_dirs = ["/usr/local/lib"]
libs = ["curl", "pthread", "m"]
flags = ["-O3", "-Wall", "-DENABLE_LOGGING"]
```

### 4. Custom Generators

Mule supports generic code generation hooks. This is useful for tools like Protobuf, FlatBuffers, or custom codegen.

```toml
[[generator]]
name = "proto"
input_extension = ".proto"
output_extension = ".pb.cc"
command = "protoc {input} --cpp_out=build/generated"
```

* `{input}`: The source file path.
* `{output}`: The target file path in `build/generated/`.
* `match_content` (optional): Only run if file contains this string.

### 5. Qt Support

Mule has first-class support for Qt projects. It automatically handles MOC, UIC, and RCC.

```toml
[qt]
enabled = true
modules = ["Core", "Widgets", "Gui"]
```

Enabling Qt automatically adds generators for:
* **MOC**: Runs on headers containing `Q_OBJECT` or `Q_GADGET`.
* **UIC**: Runs on `.ui` files.
* **RCC**: Runs on `.qrc` files.

### 6. Commands

| Command | Description |
| :--- | :--- |
| `mule new <name>` | Create a new executable project. |
| `mule new <name> --lib` | Create a new library project. |
| `mule build` | Compile the project. |
| `mule run` | Build and execute the project (if it's a binary). |
| `mule test` | Discover and run tests in the `tests/` directory. |
| `mule fetch` | Download and update dependencies. |
| `mule clean` | Remove the `build/` directory and artifacts. |

### 5. Integrated Testing

Mule looks for `.cpp` files in the `tests/` directory. Use the provided `mule_test.h` for easy assertions.

**`tests/math_test.cpp`**:
```cpp
#include "../include/mule_test.h"

MULE_TEST(addition) {
    MULE_ASSERT(1 + 1 == 2);
}

MULE_TEST(equality) {
    std::string mule = "mule";
    MULE_ASSERT(mule == "mule");
}
```

Run them with:
```bash
mule test
```

---

## 📂 Project Structure

Mule follows a standard convention:

* `src/`: Core source files.
* `include/`: Public headers.
* `tests/`: Integration tests.
* `.mule/deps/`: Managed dependencies (don't edit manually).
* `mule.lock`: Generated dependency lockfile.
* `build/`: Compilation artifacts and final binaries.

---

## 🤝 Contributing

This is an experimental tool built to improve the C++ developer experience. Feel free to open issues or submit pull requests.