cmake_minimum_required(VERSION 3.15)
project(RecloserManagement VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/RecloserManager.cpp
    src/RecloserServiceImpl.cpp
)

# Header files
set(HEADERS
    include/RecloserManager.hpp
    include/RecloserServiceImpl.hpp
)

# SQLite Sources
set(SQLITE_SOURCES
    external/sqlite/sqlite3.c
)

# SQLite Headers
set(SQLITE_HEADERS
    external/sqlite/sqlite3.h
)

# Proto generation
set(PROTO_FILE proto/recloser.proto)
get_filename_component(PROTO_PATH "${PROTO_FILE}" ABSOLUTE)
get_filename_component(PROTO_DIR "${PROTO_PATH}" PATH)

set(PROTO_SRC "${CMAKE_CURRENT_BINARY_DIR}/recloser.pb.cc")
set(PROTO_HDR "${CMAKE_CURRENT_BINARY_DIR}/recloser.pb.h")
set(GRPC_SRC "${CMAKE_CURRENT_BINARY_DIR}/recloser.grpc.pb.cc")
set(GRPC_HDR "${CMAKE_CURRENT_BINARY_DIR}/recloser.grpc.pb.h")

add_custom_command(
    OUTPUT "${PROTO_SRC}" "${PROTO_HDR}" "${GRPC_SRC}" "${GRPC_HDR}"
    COMMAND protobuf::protoc
    ARGS --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         -I "${PROTO_DIR}"
         "${PROTO_PATH}"
    DEPENDS "${PROTO_PATH}"
)

# Include directories
include_directories(include external/sqlite "${CMAKE_CURRENT_BINARY_DIR}")

# Create the executable
add_executable(${PROJECT_NAME} 
    ${SOURCES} 
    ${HEADERS} 
    ${SQLITE_SOURCES}
    "${PROTO_SRC}"
    "${GRPC_SRC}"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    pthread 
    dl
    gRPC::grpc++
    protobuf::libprotobuf
)

# Configure output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
