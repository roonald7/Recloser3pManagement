# 3P Recloser Management Application

This application is designed to manage 3P Reclosers. It uses C++20, SQLite for data persistence, and will eventually include a gRPC interface.

## Prerequisites

- **CMake** (>= 3.20)
- **C++20 compliant compiler**:
  - Windows: MSVC 2019+ (Visual Studio 2019 or later)
  - Linux: GCC 10+ or Clang 10+
  - macOS: Clang 10+ (Xcode)
- **vcpkg** - Package manager for C++ dependencies
  - Set `VCPKG_ROOT` environment variable to your vcpkg installation path
- **Ninja** - Build system (recommended)
- **SQLite** (included via amalgamation in `external/sqlite`)

### vcpkg Setup

If you don't have vcpkg installed:

```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
# Windows:
.\bootstrap-vcpkg.bat
# Linux/macOS:
./bootstrap-vcpkg.sh

# Set environment variable
# Windows (PowerShell):
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
# Linux/macOS:
export VCPKG_ROOT=/path/to/vcpkg
```

## Project Structure

- `src/`: Source code (.cpp files)
- `include/`: Header files (.hpp files)
- `external/sqlite/`: SQLite amalgamation source
- `data/`: Local SQLite database storage
- `proto/`: gRPC protocol buffer definitions
- `CMakeLists.txt`: Build system configuration
- `CMakePresets.json`: Platform-specific build configurations
- `vcpkg.json`: Dependency manifest

## How to Build

### Windows (MSVC)

#### Release Build (x64)

```powershell
# Configure
cmake --preset win-rel

# Build
cmake --build build/win-rel --config Release

# Run
.\build\win-rel\bin\RecloserManagement.exe
```

#### Debug Build (x64)

```powershell
# Configure
cmake --preset win-dbg

# Build
cmake --build build/win-dbg --config Debug

# Run
.\build\win-dbg\bin\RecloserManagement.exe
```

### Linux

#### Release Build

```bash
# Configure
cmake --preset linux-rel

# Build
cmake --build build/linux-rel

# Run
./build/linux-rel/bin/RecloserManagement
```

#### Debug Build

```bash
# Configure
cmake --preset linux-dbg

# Build
cmake --build build/linux-dbg

# Run
./build/linux-dbg/bin/RecloserManagement
```

### macOS

#### Release Build

```bash
# Configure
cmake --preset macOs-rel

# Build
cmake --build build/macOs-rel

# Run
./build/macOs-rel/bin/RecloserManagement
```

#### Debug Build

```bash
# Configure
cmake --preset macOs-dbg

# Build
cmake --build build/macOs-dbg

# Run
./build/macOs-dbg/bin/RecloserManagement
```

## Dependencies

This project uses the following libraries (managed by vcpkg):

- **gRPC** (1.44.0) - RPC framework
- **Protobuf** (3.15.8) - Protocol buffers
- **fmt** (7.1.3+) - Modern formatting library
- **clipp** (2019-04-30) - Command-line argument parser
- **nlohmann-json** (3.9.1) - JSON library

## Troubleshooting

### Windows: "pthread not found" or "dl not found"

This has been fixed in the latest CMakeLists.txt. Make sure you're using the updated version.

### vcpkg dependencies not found

Ensure `VCPKG_ROOT` environment variable is set correctly and vcpkg is bootstrapped.

### Architecture mismatch

The project is configured for x64 (64-bit) builds. Ensure your compiler toolchain matches.
