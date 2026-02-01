cmake_minimum_required(VERSION 3.15)
project(RecloserManagement VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/RecloserManager.cpp
)

# Header files
set(HEADERS
    include/RecloserManager.hpp
)

# SQLite Sources
set(SQLITE_SOURCES
    external/sqlite/sqlite3.c
)

# SQLite Headers
set(SQLITE_HEADERS
    external/sqlite/sqlite3.h
)

# Include directories
include_directories(include external/sqlite)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SQLITE_SOURCES})

# Link libraries (standard ones like pthread/dl for SQLite on Linux)
target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)

# Configure output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
