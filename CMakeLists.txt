cmake_minimum_required(VERSION 3.31)

project(Feral
    DESCRIPTION "Feral Programming Language Reference Compiler and Interpreter"
    LANGUAGES CXX)

set(PROJECT_VERSION_MAJOR "2")
set(PROJECT_VERSION_MINOR "0")
set(PROJECT_VERSION_PATCH "0")

include(FetchContent)
include(GNUInstallDirs)

if("${DISABLE_INSTALL_MSG}" STREQUAL "true")
    message("-- Install messages are now lazily shown")
    set(CMAKE_INSTALL_MESSAGE LAZY)
endif()

if("${ENABLE_ASAN}" STREQUAL "true")
    message("-- Address sanitizer is now enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

if("${ENABLE_TSAN}" STREQUAL "true")
    message("-- Thread sanitizer is now enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

# Add external path
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

if(NOT DEFINED ENV{PREFIX_DIR} OR "$ENV{PREFIX_DIR}" STREQUAL "")
    if(WIN32)
        file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/Programs/Feral" TMP_PREFIX)
        set(ENV{PREFIX_DIR} ${TMP_PREFIX})
    else()
        set(ENV{PREFIX_DIR} "$ENV{HOME}/.feral")
    endif()
endif()

set(CMAKE_INSTALL_PREFIX "$ENV{PREFIX_DIR}")
set(CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX};${CMAKE_BINARY_DIR}")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# if(APPLE)
#     set(CMAKE_MACOSX_RPATH TRUE)
# endif()
add_definitions(-DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})
message("-- Using PREFIX = ${CMAKE_INSTALL_PREFIX}")

if(WIN32)
    # Disable deprecated warnings on Windows
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options("-Wno-deprecated-declarations")
    endif()
    if("${CCACHE_PROGRAM}" STREQUAL "")
        find_program(CCACHE_PROGRAM sccache)
    endif()
else()
    if(NOT APPLE)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    endif()
    if("${CCACHE_PROGRAM}" STREQUAL "")
        find_program(CCACHE_PROGRAM ccache)
    endif()
endif()
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message("-- Using ccache: ${CCACHE_PROGRAM}")
endif()
# For macOS and BSD
if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
    include_directories(/usr/local/include)
    if(NOT APPLE)
        add_definitions(-D_WITH_GETLINE)
    endif()
endif()

string(TIMESTAMP BUILD_DATE "%a %b %d, %Y at %H:%M:%S")

execute_process(COMMAND git -C "${CMAKE_SOURCE_DIR}" config --get remote.origin.url OUTPUT_VARIABLE REPO_URL OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git -C "${CMAKE_SOURCE_DIR}" log --format=%H -n 1 OUTPUT_VARIABLE COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git -C "${CMAKE_SOURCE_DIR}" diff --quiet RESULT_VARIABLE ret)
if(ret EQUAL 0)
    set(TREE_STATUS "Clean")
else()
    set(TREE_STATUS "Dirty")
endif()

include(CheckCXXCompilerFlag)

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

if(CMAKE_COMPILER_IS_GNUCXX AND DEFINED WITH_COVERAGE)
    set(WITH_COVERAGE "-D_WITH_COVERAGE")
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    setup_target_for_coverage_gcovr_html(NAME feral_cov EXECUTABLE ${PROJECT_SOURCE_DIR}/codecov.sh ${PROJECT_SOURCE_DIR})
endif()

# Finally! The sources!!!!!

# Fetch LibCore
FetchContent_Declare(
    LibCore
    GIT_REPOSITORY https://github.com/Chirag-Khandelwal/LibCore.git
    GIT_TAG        1.1.5
)

FetchContent_MakeAvailable(LibCore)

# Set Config.inl.in template
configure_file("${PROJECT_SOURCE_DIR}/include/Config.inl.in" "${PROJECT_SOURCE_DIR}/include/Config.inl" @ONLY)

# Feral Library
file(GLOB_RECURSE INCS RELATIVE "${PROJECT_SOURCE_DIR}" "include/*.hpp")
file(GLOB_RECURSE SRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/*.cpp")
list(REMOVE_ITEM SRCS "src/Main.cpp")
add_library(libferal SHARED ${SRCS} ${INCS})
target_compile_features(libferal PUBLIC cxx_std_20)
target_link_libraries(libferal LibCore::LibCore)
# For MSVC. It requires externed variables to be specified as __declspec(dllexport/dllimport) depending on
# if the the DLL is being generated, or is being used.
target_include_directories(libferal PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(libferal PRIVATE EXPORT_FOR_DLL=true)
set_target_properties(libferal
    PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    PREFIX "" # 'lib' as prefix is not needed on linux since the name is libferal anyway
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin$<0:>" # for lib (windows)
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib" # for so (non-windows)
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin$<0:>" # for dll (windows)
)
install(TARGETS libferal)

# Feral Binary
add_executable(feral "${PROJECT_SOURCE_DIR}/src/Main.cpp")
target_compile_features(feral PUBLIC cxx_std_20)
target_include_directories(feral PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(feral libferal)
set_target_properties(feral
    PROPERTIES
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin$<0:>"
)
install(TARGETS feral
    RUNTIME
      DESTINATION bin
      COMPONENT Binaries
)
set_property(DIRECTORY "${PROJECT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT feral)

# Install CMake modules
# install(DIRECTORY "${CMAKE_SOURCE_DIR}/cmake/"
#     DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
# )

# Copy the lib/*.fer files to the build/lib directory
file(GLOB_RECURSE libModules RELATIVE "${PROJECT_SOURCE_DIR}/lib" "*.fer")
foreach(libModule ${libModules})
    get_filename_component(libModuleDir "${libModule}" DIRECTORY)
    add_custom_command(TARGET feral POST_BUILD
               COMMAND ${CMAKE_COMMAND} -E make_directory
                   "${PROJECT_BINARY_DIR}/lib/feral/${libModuleDir}"
               COMMAND ${CMAKE_COMMAND} -E copy
                "${PROJECT_SOURCE_DIR}/lib/${libModule}"
                "${PROJECT_BINARY_DIR}/lib/feral/${libModule}")
endforeach()

# Install C++ Includes
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/" "${CMAKE_SOURCE_DIR}/lib/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/include/feral"
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.inl"
)

# Install Feral Imports
install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/feral"
    FILES_MATCHING PATTERN "*.fer"
)

# Libraries (Optional)
if(NOT NOSTDLIB STREQUAL "true")
    file(GLOB_RECURSE mods RELATIVE "${PROJECT_SOURCE_DIR}" "lib/*.cpp")
    foreach(mod ${mods})
        string(SUBSTRING ${mod} 4 -1 m)
        get_filename_component(modname ${m} NAME_WE)
        get_filename_component(moddir ${m} DIRECTORY)
        set(dep "")
        # Add dependencies here for a library if needed.
        # End
        file(GLOB_RECURSE header RELATIVE "${PROJECT_SOURCE_DIR}" "lib/*${modname}.hpp")
        add_library(${modname} SHARED "${mod}" "${header}")
        target_compile_features(${modname} PUBLIC cxx_std_20)
        target_include_directories(${modname} PRIVATE "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/lib")
        target_link_libraries(${modname} libferal ${dep})
        set_target_properties(${modname}
            PROPERTIES
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            PREFIX "libferal"
            ARCHIVE_OUTPUT_NAME "libferal${modname}"
            LINK_FLAGS "${EXTRA_LD_FLAGS}"
            LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/feral/${moddir}" # for so (non-windows)
            RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/feral/${moddir}$<0:>" # for dll (windows)
            ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/feral/${moddir}$<0:>" # for lib (windows)
        )
        install(TARGETS ${modname}
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/feral/${moddir}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/feral/${moddir}$<0:>"
            RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}/feral/${moddir}$<0:>"
        )
    endforeach()
endif()

# Utility Scripts
install(DIRECTORY "${CMAKE_SOURCE_DIR}/utils/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/feral/bin"
    USE_SOURCE_PERMISSIONS
    PATTERN "*"
)