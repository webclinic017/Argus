cmake_minimum_required(VERSION 3.25)
project(Argus)

set(PACKAGE_VERSION "0.1")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_BUILD_TYPE Release)

include_directories(include)
file(GLOB SRCS src/*.cpp)
add_library(Argus SHARED ${SRCS} src/hydra.cpp include/utils_array.h)

#----external libraries----#
#fmt
include_directories(external/fmt/include)

#robin-map
find_package(tsl-robin-map REQUIRED)
include_directories(external/robin-map/include)

#Python
find_package(Python3 3.11 EXACT COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})
target_link_libraries(Argus ${Python3_LIBRARIES})

#PyBind11
find_package(pybind11 REQUIRED)
include_directories(external/pybind11/include)
#--------------------------#

# This is required for linking the library under Mac OS X. Moreover,
# the suffix ensures that the module can be found by the interpreter
# later on.
IF( APPLE )
    SET_TARGET_PROPERTIES( Argus
            PROPERTIES
            LINK_FLAGS "-undefined dynamic_lookup"
            SUFFIX     ".so"
            )
ENDIF()

# Expose asset module to Python
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Argus/FastTest)
pybind11_add_module(FastTest ${SRCS})
