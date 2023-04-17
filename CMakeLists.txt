cmake_minimum_required(VERSION 3.25)
project(Argus)

set(PACKAGE_VERSION "0.1")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_OSX_ARCHITECTURES arm64)

include_directories(include)
file(GLOB SRCS src/*.cpp)
add_library(Argus SHARED ${SRCS})

#Python dev components
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
include_directories(${Python3_INCLUDE_DIRS})
target_link_libraries(Argus ${Python3_LIBRARIES})

set_target_properties(Argus PROPERTIES SUFFIX "${PYTHON_MODULE_EXTENSION}")

#PyBind11
find_package(pybind11 REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

# The library must not have any prefix and should be located in
# a sub-folder that includes the package name. The setup will be
# more complicated otherwise.
SET_TARGET_PROPERTIES( Argus
        PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Argus"
        )

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

# Place the initialization file in the output directory for the Python
# bindings. This will simplify the installation.
CONFIGURE_FILE( Argus/__init__.py
        ${CMAKE_CURRENT_BINARY_DIR}/Argus/__init__.py
        )

# Ditto for the setup file.
CONFIGURE_FILE( Argus/setup.py
        ${CMAKE_CURRENT_BINARY_DIR}/Argus/setup.py
        )
