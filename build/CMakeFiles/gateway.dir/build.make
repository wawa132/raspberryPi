# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/pi/gateway

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/pi/gateway/build

# Include any dependencies generated for this target.
include CMakeFiles/gateway.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/gateway.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/gateway.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/gateway.dir/flags.make

CMakeFiles/gateway.dir/src/client.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/client.c.o: /home/pi/gateway/src/client.c
CMakeFiles/gateway.dir/src/client.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/gateway.dir/src/client.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/client.c.o -MF CMakeFiles/gateway.dir/src/client.c.o.d -o CMakeFiles/gateway.dir/src/client.c.o -c /home/pi/gateway/src/client.c

CMakeFiles/gateway.dir/src/client.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/client.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/client.c > CMakeFiles/gateway.dir/src/client.c.i

CMakeFiles/gateway.dir/src/client.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/client.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/client.c -o CMakeFiles/gateway.dir/src/client.c.s

CMakeFiles/gateway.dir/src/crc16.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/crc16.c.o: /home/pi/gateway/src/crc16.c
CMakeFiles/gateway.dir/src/crc16.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/gateway.dir/src/crc16.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/crc16.c.o -MF CMakeFiles/gateway.dir/src/crc16.c.o.d -o CMakeFiles/gateway.dir/src/crc16.c.o -c /home/pi/gateway/src/crc16.c

CMakeFiles/gateway.dir/src/crc16.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/crc16.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/crc16.c > CMakeFiles/gateway.dir/src/crc16.c.i

CMakeFiles/gateway.dir/src/crc16.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/crc16.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/crc16.c -o CMakeFiles/gateway.dir/src/crc16.c.s

CMakeFiles/gateway.dir/src/database.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/database.c.o: /home/pi/gateway/src/database.c
CMakeFiles/gateway.dir/src/database.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/gateway.dir/src/database.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/database.c.o -MF CMakeFiles/gateway.dir/src/database.c.o.d -o CMakeFiles/gateway.dir/src/database.c.o -c /home/pi/gateway/src/database.c

CMakeFiles/gateway.dir/src/database.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/database.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/database.c > CMakeFiles/gateway.dir/src/database.c.i

CMakeFiles/gateway.dir/src/database.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/database.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/database.c -o CMakeFiles/gateway.dir/src/database.c.s

CMakeFiles/gateway.dir/src/hash.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/hash.c.o: /home/pi/gateway/src/hash.c
CMakeFiles/gateway.dir/src/hash.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/gateway.dir/src/hash.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/hash.c.o -MF CMakeFiles/gateway.dir/src/hash.c.o.d -o CMakeFiles/gateway.dir/src/hash.c.o -c /home/pi/gateway/src/hash.c

CMakeFiles/gateway.dir/src/hash.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/hash.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/hash.c > CMakeFiles/gateway.dir/src/hash.c.i

CMakeFiles/gateway.dir/src/hash.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/hash.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/hash.c -o CMakeFiles/gateway.dir/src/hash.c.s

CMakeFiles/gateway.dir/src/main.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/main.c.o: /home/pi/gateway/src/main.c
CMakeFiles/gateway.dir/src/main.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/gateway.dir/src/main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/main.c.o -MF CMakeFiles/gateway.dir/src/main.c.o.d -o CMakeFiles/gateway.dir/src/main.c.o -c /home/pi/gateway/src/main.c

CMakeFiles/gateway.dir/src/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/main.c > CMakeFiles/gateway.dir/src/main.c.i

CMakeFiles/gateway.dir/src/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/main.c -o CMakeFiles/gateway.dir/src/main.c.s

CMakeFiles/gateway.dir/src/process.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/process.c.o: /home/pi/gateway/src/process.c
CMakeFiles/gateway.dir/src/process.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/gateway.dir/src/process.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/process.c.o -MF CMakeFiles/gateway.dir/src/process.c.o.d -o CMakeFiles/gateway.dir/src/process.c.o -c /home/pi/gateway/src/process.c

CMakeFiles/gateway.dir/src/process.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/process.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/process.c > CMakeFiles/gateway.dir/src/process.c.i

CMakeFiles/gateway.dir/src/process.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/process.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/process.c -o CMakeFiles/gateway.dir/src/process.c.s

CMakeFiles/gateway.dir/src/queue.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/queue.c.o: /home/pi/gateway/src/queue.c
CMakeFiles/gateway.dir/src/queue.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/gateway.dir/src/queue.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/queue.c.o -MF CMakeFiles/gateway.dir/src/queue.c.o.d -o CMakeFiles/gateway.dir/src/queue.c.o -c /home/pi/gateway/src/queue.c

CMakeFiles/gateway.dir/src/queue.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/queue.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/queue.c > CMakeFiles/gateway.dir/src/queue.c.i

CMakeFiles/gateway.dir/src/queue.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/queue.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/queue.c -o CMakeFiles/gateway.dir/src/queue.c.s

CMakeFiles/gateway.dir/src/rawdata.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/rawdata.c.o: /home/pi/gateway/src/rawdata.c
CMakeFiles/gateway.dir/src/rawdata.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/gateway.dir/src/rawdata.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/rawdata.c.o -MF CMakeFiles/gateway.dir/src/rawdata.c.o.d -o CMakeFiles/gateway.dir/src/rawdata.c.o -c /home/pi/gateway/src/rawdata.c

CMakeFiles/gateway.dir/src/rawdata.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/rawdata.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/rawdata.c > CMakeFiles/gateway.dir/src/rawdata.c.i

CMakeFiles/gateway.dir/src/rawdata.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/rawdata.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/rawdata.c -o CMakeFiles/gateway.dir/src/rawdata.c.s

CMakeFiles/gateway.dir/src/seed.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/seed.c.o: /home/pi/gateway/src/seed.c
CMakeFiles/gateway.dir/src/seed.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/gateway.dir/src/seed.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/seed.c.o -MF CMakeFiles/gateway.dir/src/seed.c.o.d -o CMakeFiles/gateway.dir/src/seed.c.o -c /home/pi/gateway/src/seed.c

CMakeFiles/gateway.dir/src/seed.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/seed.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/seed.c > CMakeFiles/gateway.dir/src/seed.c.i

CMakeFiles/gateway.dir/src/seed.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/seed.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/seed.c -o CMakeFiles/gateway.dir/src/seed.c.s

CMakeFiles/gateway.dir/src/server.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/server.c.o: /home/pi/gateway/src/server.c
CMakeFiles/gateway.dir/src/server.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/gateway.dir/src/server.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/server.c.o -MF CMakeFiles/gateway.dir/src/server.c.o.d -o CMakeFiles/gateway.dir/src/server.c.o -c /home/pi/gateway/src/server.c

CMakeFiles/gateway.dir/src/server.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/server.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/server.c > CMakeFiles/gateway.dir/src/server.c.i

CMakeFiles/gateway.dir/src/server.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/server.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/server.c -o CMakeFiles/gateway.dir/src/server.c.s

CMakeFiles/gateway.dir/src/systime.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/systime.c.o: /home/pi/gateway/src/systime.c
CMakeFiles/gateway.dir/src/systime.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/gateway.dir/src/systime.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/systime.c.o -MF CMakeFiles/gateway.dir/src/systime.c.o.d -o CMakeFiles/gateway.dir/src/systime.c.o -c /home/pi/gateway/src/systime.c

CMakeFiles/gateway.dir/src/systime.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/systime.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/systime.c > CMakeFiles/gateway.dir/src/systime.c.i

CMakeFiles/gateway.dir/src/systime.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/systime.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/systime.c -o CMakeFiles/gateway.dir/src/systime.c.s

CMakeFiles/gateway.dir/src/var.c.o: CMakeFiles/gateway.dir/flags.make
CMakeFiles/gateway.dir/src/var.c.o: /home/pi/gateway/src/var.c
CMakeFiles/gateway.dir/src/var.c.o: CMakeFiles/gateway.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building C object CMakeFiles/gateway.dir/src/var.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/gateway.dir/src/var.c.o -MF CMakeFiles/gateway.dir/src/var.c.o.d -o CMakeFiles/gateway.dir/src/var.c.o -c /home/pi/gateway/src/var.c

CMakeFiles/gateway.dir/src/var.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/gateway.dir/src/var.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/pi/gateway/src/var.c > CMakeFiles/gateway.dir/src/var.c.i

CMakeFiles/gateway.dir/src/var.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/gateway.dir/src/var.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/pi/gateway/src/var.c -o CMakeFiles/gateway.dir/src/var.c.s

# Object files for target gateway
gateway_OBJECTS = \
"CMakeFiles/gateway.dir/src/client.c.o" \
"CMakeFiles/gateway.dir/src/crc16.c.o" \
"CMakeFiles/gateway.dir/src/database.c.o" \
"CMakeFiles/gateway.dir/src/hash.c.o" \
"CMakeFiles/gateway.dir/src/main.c.o" \
"CMakeFiles/gateway.dir/src/process.c.o" \
"CMakeFiles/gateway.dir/src/queue.c.o" \
"CMakeFiles/gateway.dir/src/rawdata.c.o" \
"CMakeFiles/gateway.dir/src/seed.c.o" \
"CMakeFiles/gateway.dir/src/server.c.o" \
"CMakeFiles/gateway.dir/src/systime.c.o" \
"CMakeFiles/gateway.dir/src/var.c.o"

# External object files for target gateway
gateway_EXTERNAL_OBJECTS =

gateway: CMakeFiles/gateway.dir/src/client.c.o
gateway: CMakeFiles/gateway.dir/src/crc16.c.o
gateway: CMakeFiles/gateway.dir/src/database.c.o
gateway: CMakeFiles/gateway.dir/src/hash.c.o
gateway: CMakeFiles/gateway.dir/src/main.c.o
gateway: CMakeFiles/gateway.dir/src/process.c.o
gateway: CMakeFiles/gateway.dir/src/queue.c.o
gateway: CMakeFiles/gateway.dir/src/rawdata.c.o
gateway: CMakeFiles/gateway.dir/src/seed.c.o
gateway: CMakeFiles/gateway.dir/src/server.c.o
gateway: CMakeFiles/gateway.dir/src/systime.c.o
gateway: CMakeFiles/gateway.dir/src/var.c.o
gateway: CMakeFiles/gateway.dir/build.make
gateway: /usr/lib/aarch64-linux-gnu/libcrypto.so
gateway: CMakeFiles/gateway.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/pi/gateway/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Linking C executable gateway"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/gateway.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/gateway.dir/build: gateway
.PHONY : CMakeFiles/gateway.dir/build

CMakeFiles/gateway.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/gateway.dir/cmake_clean.cmake
.PHONY : CMakeFiles/gateway.dir/clean

CMakeFiles/gateway.dir/depend:
	cd /home/pi/gateway/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/pi/gateway /home/pi/gateway /home/pi/gateway/build /home/pi/gateway/build /home/pi/gateway/build/CMakeFiles/gateway.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/gateway.dir/depend

