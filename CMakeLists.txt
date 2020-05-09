cmake_minimum_required(VERSION 2.8)

project(Feral C CXX)

set(CMAKE_CXX_STANDARD 11)
if(CMAKE_VERSION VERSION_LESS "3.1")
	add_compile_options(-std=c++11)
endif()

if(DEFINED ENV{PREFIX_DIR} AND NOT "$ENV{PREFIX_DIR}" STREQUAL "")
	set(CMAKE_INSTALL_PREFIX "$ENV{PREFIX_DIR}")
endif()
add_definitions(-DBUILD_PREFIX_DIR=${CMAKE_INSTALL_PREFIX})
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/feral")
message("-- Using PREFIX = ${CMAKE_INSTALL_PREFIX}")

if(DEFINED ENV{MEM_PROFILE})
	message("-- Memory Pool profiling is true")
	add_definitions(-DMEM_PROFILE)
endif()

# add cmake_modules path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/Extra/cmake_modules")

set(FERAL_VERSION_MAJOR 0)
set(FERAL_VERSION_MINOR 0)
set(FERAL_VERSION_PATCH 2)

execute_process(COMMAND date "+%a %b %d, %Y at %H:%M:%S" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

include(CheckCXXCompilerFlag)

find_library(GMP_LIBRARY gmp REQUIRED)
find_library(GMPXX_LIBRARY gmpxx REQUIRED)
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

# For libGMP on macOS and BSD
if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
	include_directories(/usr/local/include)
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
	add_definitions(-D_WITH_GETLINE)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_definitions(-DDEBUG_MODE)
	message("-- Running in debug mode")
endif()

check_cxx_compiler_flag(-march=native COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

# Because Android has issues with dlopen()'s RTLD_GLOBAL; See:
# https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#changes-to-library-search-order
# https://android.googlesource.com/platform/bionic/+/30b17e32f0b403a97cef7c4d1fcab471fa316340/linker/linker_namespaces.cpp#100
# https://android-ndk.narkive.com/iNWj05IV/weak-symbol-linking-when-loading-dynamic-libraries
set(EXTRA_LD_FLAGS "")
if(ANDROID)
	set(EXTRA_LD_FLAGS "-z global")
endif()

# Finally! The sources!!!!!

# Set Config.hpp.in template
configure_file("${PROJECT_SOURCE_DIR}/src/Compiler/Config.hpp.in" "${PROJECT_SOURCE_DIR}/src/Compiler/Config.hpp" @ONLY)

# Feral Includes
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
)

# Install Extra headers and (cmake) modules
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/Extra/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral/Extra"
	FILES_MATCHING PATTERN "*.hpp" PATTERN "*.cmake"
)

# Feral Common
file(GLOB_RECURSE INCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/Common/*.hpp")
file(GLOB_RECURSE SRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/Common/*.cpp")

add_library(feral-common SHARED ${SRCS} ${INCS})
set_target_properties(feral-common
    PROPERTIES
    OUTPUT_NAME feralcomm
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/feral"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS feral-common
	LIBRARY
	  DESTINATION lib/feral
	  COMPONENT Libraries
	PUBLIC_HEADER
	  DESTINATION include/feral
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/Common/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral/Common"
	FILES_MATCHING PATTERN "*.hpp"
)

# Feral VM
file(GLOB_RECURSE INCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/VM/*.hpp")
file(GLOB_RECURSE SRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/VM/*.cpp")

add_library(feral-vm SHARED ${SRCS} ${INCS})
target_link_libraries(feral-vm feral-common ${CMAKE_DL_LIBS} ${MPFR_LIBRARIES} ${GMPXX_LIBRARY} ${GMP_LIBRARY})
set_target_properties(feral-vm
    PROPERTIES
    OUTPUT_NAME feralvm
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/feral"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS feral-vm
	LIBRARY
	  DESTINATION lib/feral
	  COMPONENT Libraries
	PUBLIC_HEADER
	  DESTINATION include/feral
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/VM/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral/VM"
	FILES_MATCHING PATTERN "*.hpp"
)

# Feral Compiler
file(GLOB_RECURSE INCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/Compiler/*.hpp")
file(GLOB_RECURSE SRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/Compiler/*.cpp")
list(REMOVE_ITEM SRCS "src/Compiler/Main.cpp")

add_library(feral-compiler SHARED ${SRCS} ${INCS})
target_link_libraries(feral-compiler feral-common feral-vm)
set_target_properties(feral-compiler
    PROPERTIES
    OUTPUT_NAME feralcomp
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/feral"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS feral-compiler
	LIBRARY
	  DESTINATION lib/feral
	  COMPONENT Libraries
	PUBLIC_HEADER
	  DESTINATION include/feral
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/Compiler/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral/Compiler"
	FILES_MATCHING PATTERN "*.hpp"
)

# Feral Binary
add_executable(feral-bin "${PROJECT_SOURCE_DIR}/src/Compiler/Main.cpp")
target_link_libraries(feral-bin feral-compiler feral-vm)
set_target_properties(feral-bin
	PROPERTIES
	OUTPUT_NAME feral
	LINK_FLAGS "${EXTRA_LD_FLAGS}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS feral-bin
	RUNTIME
	  DESTINATION bin
	  COMPONENT Binaries
)

# Libraries
file(GLOB_RECURSE mods RELATIVE "${PROJECT_SOURCE_DIR}" "library/*.cpp")
foreach(m ${mods})
	get_filename_component(mod ${m} NAME_WE)
	get_filename_component(moddir ${m} DIRECTORY)
	string(SUBSTRING ${moddir} 7 -1 moddir)
	add_library(${mod} SHARED "${m}")
	target_link_libraries(${mod} feral-vm)
	set_target_properties(${mod}
	    PROPERTIES
	    PREFIX "libferal"
	    LINK_FLAGS "${EXTRA_LD_FLAGS}"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/feral/${moddir}"
	    INSTALL_RPATH_USE_LINK_PATH TRUE
	)
	install(TARGETS ${mod}
		LIBRARY
		  DESTINATION lib/feral/${moddir}
		  COMPONENT Libraries
	)
endforeach()
