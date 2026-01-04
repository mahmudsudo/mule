##  Mule: A Minimalist C++ Build System & Package Manager

Mule is a lightweight, convention-over-configuration build tool for C++ developers. Inspired by Rust's Cargo, Mule simplifies project scaffolding, handles incremental builds, and manages dependencies directly from Git repositories without the complexity of CMake. IT was built with developers in mind 

### 🚀 Key Features

* **Zero Configuration:** Start a project in seconds.
* **Incremental Builds:** Only recompiles files that have changed, saving you time.
* **Git-Integrated Dependency Management:** Fetch and link libraries directly from GitHub.
* **Native TOML Configuration:** Manage your project settings in a human-readable format.
* **Cross-Platform Ready:** Built with modern C++17 `std::filesystem`.

---

## 🛠 Installation

### Prerequisites

* A C++17 compatible compiler (`g++` or `clang++`).
* `git` (for dependency management).

## 📥 Binary Installation (Quickest)
If you don't want to build from source, download the latest pre-compiled binary:

Download the mule binary from the Releases page.

Make it executable and move it to your path:

```Bash

chmod +x mule
sudo mv mule /usr/local/bin/
```
## Verify installation:

```Bash

mule --version

```


### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/mahmudsudo/mule.git
cd mule

```


2. Compile the binary:
```bash
g++ -O3 -static main.cpp cli.cpp -o mule

```


3. Move to your PATH:
```bash
sudo mv mule /usr/local/bin/

```



---

## 📖 Usage Guide

### 1. Create a New Project

Generate a standard directory structure instantly:

```bash
mule new my_project

```

### 2. Add Dependencies

Open `mule.toml` and add your favorite libraries:

```toml
[package]
name = "my_app"
standard = "20"

[dependencies]
fmt = "https://github.com/fmtlib/fmt.git"

```

### 3. Build and Run

Mule automatically fetches dependencies, compiles your source, and links the binary:

```bash
mule run

```

### 4. Clean Build Artifacts

```bash
mule clean

```

---

## 📂 Project Structure Enforced by Mule

Mule expects the following layout (which it creates for you):

* `src/`: All your `.cpp` source files.
* `include/`: Your public `.h`/`.hpp` header files.
* `.mule/deps/`: Where your external libraries live.
* `build/`: Compiled object files and final executable.

---

## 🛠 Technical Internals

Mule uses a **Section-Aware TOML Parser** and a **Task Orchestrator** to manage the compilation pipeline.

---

## 🤝 Contributing

This is an experimental tool built to improve the C++ developer experience. Feel free to open issues or submit pull requests