cmake_minimum_required(VERSION 3.25)

project(devdash
    VERSION 0.1.0
    DESCRIPTION "Modular automotive dashboard framework"
    LANGUAGES CXX
)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(DEVDASH_BUILD_TESTS "Build tests" ON)
option(DEVDASH_ENABLE_SANITIZERS "Enable ASan/UBSan in Debug builds" ON)
option(DEVDASH_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
option(DEVDASH_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# Include CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Sanitizers)
include(StaticAnalysis)
include(FetchDependencies)

# Find Qt (Ubuntu 24.04 ships with 6.4.2)
find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Quick
    Qml
    Multimedia
    SerialBus
    Test
)
qt_standard_project_setup()

# Add subdirectories
add_subdirectory(src)

if(DEVDASH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Format target
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking code formatting"
    )
    add_custom_target(format-fix
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Fixing code formatting"
    )
endif()

# clang-tidy targets
find_program(CLANG_TIDY_BIN clang-tidy)
if(CLANG_TIDY_BIN)
    file(GLOB_RECURSE ALL_SOURCE_FILES_TIDY
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
    )
    add_custom_target(clang-tidy
        COMMAND ${CLANG_TIDY_BIN} -p ${CMAKE_BINARY_DIR} ${ALL_SOURCE_FILES_TIDY}
        COMMENT "Running clang-tidy checks"
    )
    add_custom_target(clang-tidy-fix
        COMMAND ${CLANG_TIDY_BIN} -p ${CMAKE_BINARY_DIR} --fix ${ALL_SOURCE_FILES_TIDY}
        COMMENT "Running clang-tidy with --fix"
    )
endif()
