# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.26

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.26.0/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.26.0/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/nathantormaschy/CLionProjects/Argus

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/nathantormaschy/CLionProjects/Argus/build

# Include any dependencies generated for this target.
include CMakeFiles/asset.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/asset.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/asset.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/asset.dir/flags.make

CMakeFiles/asset.dir/src/asset.cpp.o: CMakeFiles/asset.dir/flags.make
CMakeFiles/asset.dir/src/asset.cpp.o: /Users/nathantormaschy/CLionProjects/Argus/src/asset.cpp
CMakeFiles/asset.dir/src/asset.cpp.o: CMakeFiles/asset.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/nathantormaschy/CLionProjects/Argus/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/asset.dir/src/asset.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/asset.dir/src/asset.cpp.o -MF CMakeFiles/asset.dir/src/asset.cpp.o.d -o CMakeFiles/asset.dir/src/asset.cpp.o -c /Users/nathantormaschy/CLionProjects/Argus/src/asset.cpp

CMakeFiles/asset.dir/src/asset.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/asset.dir/src/asset.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/nathantormaschy/CLionProjects/Argus/src/asset.cpp > CMakeFiles/asset.dir/src/asset.cpp.i

CMakeFiles/asset.dir/src/asset.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/asset.dir/src/asset.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/nathantormaschy/CLionProjects/Argus/src/asset.cpp -o CMakeFiles/asset.dir/src/asset.cpp.s

# Object files for target asset
asset_OBJECTS = \
"CMakeFiles/asset.dir/src/asset.cpp.o"

# External object files for target asset
asset_EXTERNAL_OBJECTS =

asset.cpython-311-darwin.so: CMakeFiles/asset.dir/src/asset.cpp.o
asset.cpython-311-darwin.so: CMakeFiles/asset.dir/build.make
asset.cpython-311-darwin.so: CMakeFiles/asset.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/nathantormaschy/CLionProjects/Argus/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared module asset.cpython-311-darwin.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/asset.dir/link.txt --verbose=$(VERBOSE)
	/Library/Developer/CommandLineTools/usr/bin/strip -x /Users/nathantormaschy/CLionProjects/Argus/build/asset.cpython-311-darwin.so

# Rule to build all files generated by this target.
CMakeFiles/asset.dir/build: asset.cpython-311-darwin.so
.PHONY : CMakeFiles/asset.dir/build

CMakeFiles/asset.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/asset.dir/cmake_clean.cmake
.PHONY : CMakeFiles/asset.dir/clean

CMakeFiles/asset.dir/depend:
	cd /Users/nathantormaschy/CLionProjects/Argus/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/nathantormaschy/CLionProjects/Argus /Users/nathantormaschy/CLionProjects/Argus /Users/nathantormaschy/CLionProjects/Argus/build /Users/nathantormaschy/CLionProjects/Argus/build /Users/nathantormaschy/CLionProjects/Argus/build/CMakeFiles/asset.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/asset.dir/depend

