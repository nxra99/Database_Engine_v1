cmake_minimum_required(VERSION 3.28.3)
project(toydb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Catch2 for testing
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.3
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# Library target — holds all logic, no main()
add_library(toydb_lib
  # src/storage/page.cpp
  src/storage/disk_manager.cpp
  src/Status.h
  src/StatusOr.h
  src/Status.cpp
  src/storage/header_files/page.h
  src/storage/header_files/slottedpage.h
  src/storage/page.cpp
  src/storage/slottedpage.cpp
  src/storage/header_files/buffer_pool.h
  src/storage/buffer_pool.cpp
  # src/storage/buffer_pool.cpp
  src/serialisation.h
  src/typdefs.h
  src/storage/header_files/heap_file.h
  src/storage/heap_file.cpp
)

target_include_directories(toydb_lib PUBLIC
  src
  src/storage/header_files
)

target_compile_options(toydb_lib PRIVATE
  -Wall -Wextra -Wpedantic
  -fsanitize=address -fsanitize=undefined
  -g -O0
)
target_link_options(toydb_lib PUBLIC
  -fsanitize=address -fsanitize=undefined
)

# Main executable (re-enable once src/main.cpp exists)
# add_executable(toydb src/main.cpp)
# target_link_libraries(toydb PRIVATE toydb_lib)

# Tests
enable_testing()
include(CTest)
include(Catch)
add_subdirectory(src/tests)