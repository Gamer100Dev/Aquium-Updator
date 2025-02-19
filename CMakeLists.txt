# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Define the project name
project(AquiumUpdater)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)

# Use pkg-config to get Qt5 flags
find_package(PkgConfig REQUIRED)
pkg_check_modules(Qt5 REQUIRED Qt5Core Qt5Widgets Qt5Gui)

# Output the directories found by pkg-config
message(STATUS "Qt5 Include Directories: ${Qt5_INCLUDE_DIRS}")
message(STATUS "Qt5 Libraries: ${Qt5_LIBRARIES}")

# Include directories for Qt5
include_directories(${Qt5_INCLUDE_DIRS})

# Add the source file
set(SRC src/main.cpp)

# Define the executable target
add_executable(AquiumUpdater ${SRC})

# Link the required Qt5 libraries
target_link_libraries(AquiumUpdater ${Qt5_LIBRARIES})

# Optionally, set the output path for the executable (if desired)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
