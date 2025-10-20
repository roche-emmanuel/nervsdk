cmake_minimum_required(VERSION 3.20)

project(NervSDK)
set(PROJECT_VERSION 22.7)

# Include the utils definitions:
include(cmake/utils.cmake)

# Convenient setup of the compiler type:
detect_compiler()

message(STATUS "Building ${CMAKE_BUILD_TYPE} version of NervSDK")

setup_compiler_flags()

# prepare the source folder:
set(SRC_DIR ${PROJECT_SOURCE_DIR}/sources)

# Add the sources folder
add_subdirectory(sources)
