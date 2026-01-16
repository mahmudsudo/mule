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

### ⚡ Quick Install (Linux/macOS)
The easiest way to install Mule is via the official installer script:

```bash
curl -fsSL https://raw.githubusercontent.com/mahmudsudo/mule/main/install.sh | bash
```

### 🍺 Homebrew (macOS/Linux)
You can install Mule using our Homebrew formula:

```bash
brew install https://raw.githubusercontent.com/mahmudsudo/mule/main/formula/mule.rb
```

### 🛍 Snap Store (Linux)
Mule is available as a Snap package:

```bash
sudo snap install mule
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
* CUDA Projects

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

> [!NOTE]
> **CMake Integration:** Mule automatically detects `CMakeLists.txt` in your dependencies. It will run the CMake build process and automatically link the generated libraries, as well as discover `include` directories.

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
flags = ["-O3", "-Wall"]
defines = ["ENABLE_LOGGING", "VERSION_MAJOR=1"]
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

### 6. CUDA Support

Mule supports CUDA projects out of the box. It detects `nvcc` and handles `.cu` file compilation and linking.

```toml
[cuda]
enabled = true
```

* **Automatic Linking**: When CUDA is enabled, Mule automatically links against the CUDA runtime (`cudart`).
* **Source Discovery**: Mule automatically finds and compiles `.cu` files in your `src/` directory.

### 7. Commands

| Command | Description |
| :--- | :--- |
| `mule new <name>` | Create a new executable project. |
| `mule new <name> --lib` | Create a new library project. |
| `mule build` | Compile the project. |
| `mule run` | Build and execute the project (if it's a binary). |
| `mule test` | Discover and run tests (unit and integration). |
| `mule fetch` | Download and update dependencies. |
| `mule clean` | Remove the `build/` directory and artifacts. |

### 5. Integrated Testing (Cargo-style)

Mule follows Rust's Cargo convention for a professional C++ testing experience.

#### Unit Tests
Unit tests are co-located with your source code. Any file in `src/` ending with `_test.cpp` is treated as a unit test and compiled with your library code.

**`src/utils_test.cpp`**:
```cpp
#include "mule_test.h"
#include "utils.h"

MULE_TEST(addition) {
    MULE_ASSERT(add(1, 1) == 2);
}
```

#### Integration Tests
Standalone tests reside in the `tests/` directory. Each `.cpp` file here is compiled as its own separate executable, ensuring you only use the public interface of your library.

**`tests/api_test.cpp`**:
```cpp
#include "mule_test.h"
#include "my_lib.h"

int main() {
    // Integration tests can have their own main or use MULE_TEST with a generated runner
    return 0; 
}
```

Run all tests with a single command:
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