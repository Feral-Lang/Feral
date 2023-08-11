cmake_minimum_required(VERSION 3.24.3)

project(Feral C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${DISABLE_INSTALL_MSG}" STREQUAL "true")
	message("-- Install messages are now lazily shown")
	set(CMAKE_INSTALL_MESSAGE LAZY)
endif()

if(NOT DEFINED ENV{PREFIX_DIR} OR "$ENV{PREFIX_DIR}" STREQUAL "")
	set(ENV{PREFIX_DIR} "$ENV{HOME}/.feral")
endif()
set(CMAKE_INSTALL_PREFIX "$ENV{PREFIX_DIR}")
set(CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX};${CMAKE_BINARY_DIR}")
add_definitions(-DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})
message("-- Using PREFIX = ${CMAKE_INSTALL_PREFIX}")

# add external path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/external")

set(SOURCE_VERSION_MAJOR 0)
set(SOURCE_VERSION_MINOR 0)
set(SOURCE_VERSION_PATCH 1)

execute_process(COMMAND date "+%a %b %d, %Y at %H:%M:%S" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git config --get remote.origin.url OUTPUT_VARIABLE REPO_URL OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git log --format=%H -n 1 OUTPUT_VARIABLE COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git diff --quiet RESULT_VARIABLE ret)
if(ret EQUAL 0)
	set(TREE_STATUS "Clean")
else()
	set(TREE_STATUS "Dirty")
endif()

set(ORIGIN "$ORIGIN")
if(APPLE)
	set(ORIGIN "@loader_path")
endif()
list(APPEND CMAKE_INSTALL_RPATH "${ORIGIN}/../lib/feral" "${ORIGIN}/.." "${ORIGIN}/.")
set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

include(CheckCXXCompilerFlag)

# For libGMP on macOS and BSD
if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
	include_directories(/usr/local/include)
endif()

find_library(GMP_LIBRARY gmp REQUIRED)
find_package(MPFR REQUIRED)
if(NOT MPFR_FOUND)
	message(FATAL_ERROR "MPFR library is required but missing")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
	message("-- Using thread flags: -pthread")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pthread")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread")
else()
	message("-- Using thread flags: ${CMAKE_THREAD_LIBS_INIT}")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    message("-- Using ccache")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
	add_definitions(-D_WITH_GETLINE)
endif()

# Generally, disabled for CI purposes
# Also needed when running valgrind as valgrind does not support instruction bytes for Vector<bool>
if(NOT DEFINED ENV{DISABLE_MARCH_NATIVE} OR "$ENV{DISABLE_MARCH_NATIVE}" STREQUAL "")
	check_cxx_compiler_flag(-march=native COMPILER_SUPPORTS_MARCH_NATIVE)
	if(COMPILER_SUPPORTS_MARCH_NATIVE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
	endif()
endif()

# Force DWARF4 as valgrind does not support anything newer (clang-14 defaults to DWARF5).
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
	check_cxx_compiler_flag(-gdwarf-4 COMPILER_SUPPORTS_GDWARF4)
	if(COMPILER_SUPPORTS_GDWARF4)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
	endif()
endif()

# Because Android has issues with dlopen()'s RTLD_GLOBAL; See:
# https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#changes-to-library-search-order
# https://android.googlesource.com/platform/bionic/+/30b17e32f0b403a97cef7c4d1fcab471fa316340/linker/linker_namespaces.cpp#100
# https://android-ndk.narkive.com/iNWj05IV/weak-symbol-linking-when-loading-dynamic-libraries
set(EXTRA_LD_FLAGS "")
if(ANDROID)
	set(EXTRA_LD_FLAGS "-z global")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND DEFINED WITH_COVERAGE)
    set(WITH_COVERAGE "-D_WITH_COVERAGE")
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    setup_target_for_coverage_gcovr_html(NAME feral_cov EXECUTABLE ${PROJECT_SOURCE_DIR}/codecov.sh ${PROJECT_SOURCE_DIR})
endif()

# Finally! The sources!!!!!

# Set Config.hpp.in template
configure_file("${PROJECT_SOURCE_DIR}/include/Config.hpp.in" "${PROJECT_SOURCE_DIR}/include/Config.hpp" @ONLY)

# Install CMake modules
install(DIRECTORY "${CMAKE_SOURCE_DIR}/external/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/external"
)

# Install C++ Includes
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral"
	FILES_MATCHING PATTERN "*.hpp"
)

# Install Feral Imports
install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral"
	FILES_MATCHING PATTERN "*.fer" PATTERN "*.hpp"
	PATTERN "lib/Core" EXCLUDE
)

# Feral Compiler Library
file(GLOB_RECURSE INCS RELATIVE "${PROJECT_SOURCE_DIR}" "include/*.hpp")
file(GLOB_RECURSE SRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/*.cpp")
list(REMOVE_ITEM SRCS "src/Main.cpp")
add_library(feral SHARED ${SRCS})
target_include_directories(feral PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(feral ${MPFR_LIBRARIES} ${GMP_LIBRARY})
set_target_properties(feral
    PROPERTIES
    PREFIX "lib"
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/feral/"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS feral
	LIBRARY
	  DESTINATION lib/feral/
	  COMPONENT Libraries
)

# Feral Compiler
add_executable(feral-compiler "${PROJECT_SOURCE_DIR}/src/Main.cpp" ${SRCS} ${INCS})
target_include_directories(feral-compiler PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(feral-compiler feral)
set_target_properties(feral-compiler
	PROPERTIES
	OUTPUT_NAME feral
	LINK_FLAGS "${EXTRA_LD_FLAGS}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS feral-compiler
	RUNTIME
	  DESTINATION bin
	  COMPONENT Binaries
)

# Libraries
file(GLOB_RECURSE mods RELATIVE "${PROJECT_SOURCE_DIR}" "lib/*.cpp")
foreach(mod ${mods})
	string(SUBSTRING ${mod} 4 -1 m)
	get_filename_component(modname ${m} NAME_WE)
	get_filename_component(moddir ${m} DIRECTORY)
	if(NOT "${moddir}" STREQUAL "")
		string(APPEND moddir "/")
	endif()
	# add dependencies
	set(dep "")
	if("${modname}" STREQUAL "Bytebuffer")
		set(dep "BytebufferType")
	endif()
	if("${modname}" STREQUAL "FS")
		set(dep "BytebufferType" "FSType")
	endif()
	if("${modname}" STREQUAL "IO")
		set(dep "FS" "FSType")
	endif()
	if("${modname}" STREQUAL "Lang")
		set(dep "StructType")
	endif()
	if("${modname}" STREQUAL "MapType")
		set(dep "StructType")
	endif()
	if("${modname}" STREQUAL "Map")
		set(dep "MapType")
	endif()
	if("${modname}" STREQUAL "MultiProc")
		set(dep "MultiProcType")
	endif()
	if("${modname}" STREQUAL "Ptr")
		set(dep "PtrType")
	endif()
	if("${modname}" STREQUAL "Stat")
		set(dep "StructType")
	endif()
	if("${modname}" STREQUAL "Vec")
		set(dep "VecType")
	endif()
	add_library(${modname} SHARED "${mod}")
	target_include_directories(${modname} PRIVATE ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/lib)
	target_link_libraries(${modname} feral ${dep})
	set_target_properties(${modname}
	    PROPERTIES
	    PREFIX "libferal"
	    LINK_FLAGS "${EXTRA_LD_FLAGS}"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/feral/${moddir}"
	    INSTALL_RPATH_USE_LINK_PATH TRUE
	)
	install(TARGETS ${modname}
		LIBRARY
		  DESTINATION lib/feral/${moddir}
		  COMPONENT Libraries
	)
endforeach()

# Utility Scripts
install(DIRECTORY "${CMAKE_SOURCE_DIR}/utils/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
	USE_SOURCE_PERMISSIONS
	PATTERN "*"
)