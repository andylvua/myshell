cmake_minimum_required(VERSION 3.15)

#! Check every comment after the "#!"

#! CHANGE YOUR PROJECT NAME
#  It is used as your project's main executable name. 
set(PROJECT_NAME myshell)
project(${PROJECT_NAME} CXX) # project(${PROJECT_NAME} C CXX ASM)

set(CMAKE_CXX_STANDARD 20)

##########################################################
# User configurable options of the template
##########################################################
# Note: symbols like WARNINGS_AS_ERRORS in configuration are intentionally variables
# and not CMake options --using options creates too many problems for students.

#! It is a good practice to set "WARNINGS_AS_ERRORS" ON,
#  but sometimes it creates too much trouble, so default is OFF.
set(WARNINGS_AS_ERRORS ON)

#! Always use PVS Studio while developing.
set(ENABLE_PVS_STUDIO OFF)

#! Select appropriate sanitizers.
#  Definitely enable sanitizers while developing.
#  Disable it for the production builds and before submitting for grading.
#  Only one of the Memory (MSAN), Address (ASAN), or Thread (TSan)
#  sanitizers is applicable at the time -- the first defined.

#! UndefinedBehaviorSanitizer (UBSan).
#  Info: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
set(ENABLE_UBSan OFF)
#! AddressSanitizer -- detects use after free or after scope exit,
#  memory overflows and leaks.
#  Info: https://github.com/google/sanitizers/wiki/AddressSanitizer
set(ENABLE_ASAN OFF)
#! ThreadSanitizer -- detects data races.
#  Info: https://clang.llvm.org/docs/ThreadSanitizer.html
set(ENABLE_TSan OFF)
#! MemorySanitizer -- detects uninitialized memory reads
#  Info: https://github.com/google/sanitizers/wiki/MemorySanitizer
set(ENABLE_MSAN OFF)

#! Be default -- build release version if not specified otherwise.
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

# Warnings as errors should be imported here -- do not move this line
include(cmake/CompilerWarnings.cmake)

##########################################################
# Project files, packages, libraries and so on
##########################################################

#! Project main executable source compilation

# Define sources and includes via globbing for convenience, but exclude src/external from sources
file(GLOB_RECURSE SOURCES src/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX "src/external/.*")
file(GLOB_RECURSE HEADERS inc/*)

# Add external programs
include(cmake/ExternalPrograms.cmake)
message(STATUS "External programs: ${EXTERNAL}")
message(STATUS "External programs binary path: ${EXTERNAL_BIN_PATH}")

# Configure external programs header
configure_file(src/templates/msh_external.h.in ${CMAKE_BINARY_DIR}/generated/msh_external.h)
# Configure history file path
set(MSH_HISTORY_PATH "${CMAKE_BINARY_DIR}/msh/.msh_history")
message(STATUS "History file path: ${MSH_HISTORY_PATH}. Override if needed.")
configure_file(src/templates/msh_history.h.in ${CMAKE_BINARY_DIR}/generated/msh_history.h)

# Specify the wildcard expansion behavior for double-quoted strings
# if ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION is ON, then double-quoted strings are expanded
# if ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION is OFF, then double-quoted strings are not expanded
set(ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION ON)

if (ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION)
	add_compile_definitions(ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION)
endif ()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

#! Put path to your project headers
target_include_directories(${PROJECT_NAME} PRIVATE inc)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)

#! Add external packages
find_package(Boost 1.71.0 REQUIRED COMPONENTS filesystem system program_options)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

# readline library
target_link_libraries(${PROJECT_NAME} readline)

##########################################################
# Fixed CMakeLists.txt part
##########################################################

INSTALL(PROGRAMS
		$<TARGET_FILE:${PROJECT_NAME}> # ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
		DESTINATION bin)

# Define ALL_TARGETS variable to use in PVS and Sanitizers
set(ALL_TARGETS ${PROJECT_NAME})

# Include CMake setup
include(cmake/main-config.cmake)
