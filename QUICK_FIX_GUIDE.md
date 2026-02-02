# Quick Fix Guide for Windows MSVC Build

## Critical Issue: Build Will Fail ❌

Your `CMakeLists.txt` links Linux-specific libraries that don't exist on Windows.

### The Problem

**File:** `CMakeLists.txt` (lines 68-73)

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE 
    pthread    # ❌ Linux only
    dl         # ❌ Linux only
    gRPC::grpc++
    protobuf::libprotobuf
)
```

### The Fix

Replace the above with:

```cmake
# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    gRPC::grpc++
    protobuf::libprotobuf
)

# Platform-specific libraries (only for Linux)
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
endif()
```

---

## Recommended Improvements

### 1. Switch to x64 Architecture

**Current:** `x86-windows-static` (32-bit)  
**Recommended:** `x64-windows-static` (64-bit)

**File:** `CMakePresets.json` (line 48)

**Change:**

```json
"VCPKG_TARGET_TRIPLET": "x64-windows-static"
```

**Add:**

```json
"CMAKE_GENERATOR_PLATFORM": "x64"
```

### 2. Add Windows Debug Preset

Add this to `configurePresets` array in `CMakePresets.json`:

```json
{
    "name": "win-dbg",
    "displayName": "Windows x64 Debug Config",
    "generator": "Ninja",
    "inherits": "default",
    "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_MSVC_RUNTIME_LIBRARY": "MultiThreadedDebug",
        "VCPKG_TARGET_TRIPLET": "x64-windows-static",
        "CMAKE_GENERATOR_PLATFORM": "x64"
    }
}
```

Add this to `buildPresets` array:

```json
{
    "name": "win-dbg",
    "configurePreset": "win-dbg"
}
```

### 3. Add MSVC Compiler Flags

Add this to `CMakeLists.txt` after the `add_executable()` command:

```cmake
# MSVC-specific compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /W4                    # Warning level 4
        /MP                    # Multi-processor compilation
        /utf-8                 # UTF-8 encoding
        /Zc:preprocessor       # Standards-conforming preprocessor
    )
    
    # Suppress SQLite warnings
    set_source_files_properties(${SQLITE_SOURCES} PROPERTIES
        COMPILE_FLAGS "/wd4996 /wd4244 /wd4267"
    )
endif()
```

### 4. Modernize Include Directories

**Replace:**

```cmake
include_directories(include external/sqlite "${CMAKE_CURRENT_BINARY_DIR}")
```

**With:**

```cmake
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    external/sqlite
    "${CMAKE_CURRENT_BINARY_DIR}"
)
```

---

## Build Instructions

### After Applying Fixes

```powershell
# Clean old build
Remove-Item -Recurse -Force build/win-rel

# Configure (Release)
cmake --preset win-rel

# Build (Release)
cmake --build build/win-rel --config Release

# Run
.\build\win-rel\bin\RecloserManagement.exe
```

### For Debug Builds (after adding win-dbg preset)

```powershell
# Configure (Debug)
cmake --preset win-dbg

# Build (Debug)
cmake --build build/win-dbg --config Debug

# Run
.\build\win-dbg\bin\RecloserManagement.exe
```

---

## Priority Order

1. ✅ **Fix library linking** (Critical - build will fail without this)
2. ✅ **Switch to x64** (Highly recommended for modern Windows)
3. ✅ **Add debug preset** (Essential for development)
4. ✅ **Add MSVC flags** (Improves code quality and build speed)
5. ✅ **Modernize includes** (Best practice)

---

## Need Help?

See `WINDOWS_MSVC_BUILD_REVIEW.md` for detailed explanations and rationale.
