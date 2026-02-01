# 3P Recloser Management Application

This application is designed to manage 3P Reclosers. It uses C++20, SQLite for data persistence, and will eventually include a gRPC interface.

## Prerequisites

- CMake (>= 3.15)
- C++20 compliant compiler (GCC 10+, Clang 10+, or MSVC 2019+)
- SQLite (included via amalgamation in `external/sqlite`)

## Project Structure

- `src/`: Source code (.cpp files)
- `include/`: Header files (.hpp files)
- `external/sqlite/`: SQLite amalgamation source
- `data/`: Local SQLite database storage
- `CMakeLists.txt`: Build system configuration

## How to Build

```bash
mkdir build
cd build
cmake ..
make
```

## How to Run

```bash
./bin/RecloserManagement
```
