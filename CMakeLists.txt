#scc -M external -i cpp,py,h
cmake_minimum_required(VERSION 3.25)
set(PACKAGE_VERSION "0.1")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES arm64)
message("Compiler: ${CMAKE_CXX_COMPILER}")

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-S, --gc-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

# Set stack protection flag
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")

# Enable Position Independent Executable mode
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pie -fPIE")


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Argus/FastTest)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
project(Argus)

#----external libraries----#
#fmt
include_directories(external/fmt/include)
add_subdirectory(external/fmt)

#robin-map
find_package(tsl-robin-map REQUIRED)
include_directories(external/robin-map/include)

#Python
find_package(Python3 3.9 EXACT COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})

include_directories(include)

#PyBind11
find_package(pybind11 REQUIRED)
include_directories(external/pybind11/include)
#--------------------------#

# This is required for linking the library under Mac OS X. Moreover,
# the suffix ensures that the module can be found by the interpreter
# later on.

file(GLOB SRCS src/*.cpp)

# Expose asset module to Python
pybind11_add_module(FastTest ${SRCS})
target_link_libraries(FastTest PRIVATE fmt::fmt-header-only)