# devdash - Repository Setup Prompt

## Overview

Set up the `devdash` repository with modern C++ best practices, comprehensive tooling, and CI/CD. This is a Qt 6/QML application targeting automotive dashboards.

## Repository Structure to Create

```
devdash/
├── .github/
│   └── workflows/
│       ├── ci.yml              # Build, test, lint on PR
│       └── release.yml         # Tagged releases
├── .devcontainer/
│   ├── devcontainer.json
│   ├── Dockerfile
│   └── setup-vcan.sh
├── .clang-format
├── .clang-tidy
├── .gitignore
├── CMakeLists.txt
├── CMakePresets.json
├── CLAUDE.md                   # AI assistant instructions
├── LICENSE
├── README.md
│
├── cmake/
│   ├── CompilerWarnings.cmake
│   ├── Sanitizers.cmake
│   ├── StaticAnalysis.cmake
│   └── FetchDependencies.cmake
│
├── src/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── core/
│   │   ├── CMakeLists.txt
│   │   ├── IProtocolAdapter.h
│   │   ├── DataBroker.cpp
│   │   ├── DataBroker.h
│   │   └── ...
│   ├── adapters/
│   │   ├── CMakeLists.txt
│   │   └── ...
│   ├── cluster/
│   │   ├── CMakeLists.txt
│   │   └── qml/
│   └── headunit/
│       ├── CMakeLists.txt
│       └── qml/
│
├── tests/
│   ├── CMakeLists.txt
│   └── ...
│
├── protocols/
│   └── haltech/
│       └── haltech-can-protocol-v2.35.json
│
├── profiles/
│   ├── example-simulator.json
│   └── example-haltech.json
│
└── scripts/
    ├── setup-vcan.sh
    └── format-all.sh
```

## File Contents

### Root CMakeLists.txt

```cmake
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

# Find Qt
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Quick
    Qml
    Multimedia
    SerialBus
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
```

### CMakePresets.json

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "base",
            "hidden": true,
            "binaryDir": "${sourceDir}/build/${presetName}",
            "cacheVariables": {
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
            }
        },
        {
            "name": "dev",
            "displayName": "Development",
            "inherits": "base",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "DEVDASH_BUILD_TESTS": "ON",
                "DEVDASH_ENABLE_SANITIZERS": "ON",
                "DEVDASH_ENABLE_CLANG_TIDY": "OFF"
            }
        },
        {
            "name": "release",
            "displayName": "Release",
            "inherits": "base",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "DEVDASH_BUILD_TESTS": "OFF",
                "DEVDASH_ENABLE_SANITIZERS": "OFF",
                "CMAKE_INTERPROCEDURAL_OPTIMIZATION": "ON"
            }
        },
        {
            "name": "ci",
            "displayName": "CI Build",
            "inherits": "base",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "DEVDASH_BUILD_TESTS": "ON",
                "DEVDASH_ENABLE_SANITIZERS": "ON",
                "DEVDASH_ENABLE_CLANG_TIDY": "ON",
                "DEVDASH_WARNINGS_AS_ERRORS": "ON"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "dev",
            "configurePreset": "dev"
        },
        {
            "name": "release",
            "configurePreset": "release"
        },
        {
            "name": "ci",
            "configurePreset": "ci"
        }
    ],
    "testPresets": [
        {
            "name": "dev",
            "configurePreset": "dev",
            "output": {
                "outputOnFailure": true
            }
        },
        {
            "name": "ci",
            "configurePreset": "ci",
            "output": {
                "outputOnFailure": true
            }
        }
    ]
}
```

### cmake/CompilerWarnings.cmake

```cmake
# Compiler warning flags for different compilers

function(set_project_warnings target_name)
    set(CLANG_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wcast-align
        -Wunused
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
    )

    set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wuseless-cast
    )

    if(DEVDASH_WARNINGS_AS_ERRORS)
        list(APPEND CLANG_WARNINGS -Werror)
        list(APPEND GCC_WARNINGS -Werror)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS ${GCC_WARNINGS})
    else()
        message(WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}'")
    endif()

    target_compile_options(${target_name} PRIVATE ${PROJECT_WARNINGS})
endfunction()
```

### cmake/Sanitizers.cmake

```cmake
# Address and Undefined Behavior Sanitizers

function(enable_sanitizers target_name)
    if(NOT DEVDASH_ENABLE_SANITIZERS)
        return()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target_name} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${target_name} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()
```

### cmake/StaticAnalysis.cmake

```cmake
# clang-tidy integration

function(enable_clang_tidy target_name)
    if(NOT DEVDASH_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY}"
        )
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
endfunction()
```

### cmake/FetchDependencies.cmake

```cmake
# External dependencies via FetchContent

include(FetchContent)

# Catch2 for testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)

# nlohmann/json for config parsing
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(Catch2 nlohmann_json)

# Add Catch2 CMake module path for catch_discover_tests
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
```

### .clang-format

```yaml
---
Language: Cpp
BasedOnStyle: LLVM

# Indentation
IndentWidth: 4
TabWidth: 4
UseTab: Never
IndentCaseLabels: false
NamespaceIndentation: None

# Braces
BreakBeforeBraces: Attach
Cpp11BracedListStyle: true

# Line length
ColumnLimit: 100

# Includes
IncludeBlocks: Regroup
IncludeCategories:
  # Project headers
  - Regex: '^".*'
    Priority: 1
  # Qt headers
  - Regex: '^<Q.*'
    Priority: 2
  # System/external headers
  - Regex: '^<.*'
    Priority: 3

SortIncludes: CaseSensitive

# Alignment
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: None
AlignConsecutiveDeclarations: None
AlignOperands: Align
AlignTrailingComments: true

# Breaking
AllowShortBlocksOnASingleLine: Empty
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AlwaysBreakTemplateDeclarations: Yes
BinPackArguments: true
BinPackParameters: true
BreakConstructorInitializers: BeforeColon

# Pointers/References
DerivePointerAlignment: false
PointerAlignment: Left

# Spaces
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesInAngles: Never
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false

# Other
ReflowComments: true
SortUsingDeclarations: true
```

### .clang-tidy

```yaml
---
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-identifier-length,
  -cppcoreguidelines-avoid-magic-numbers,
  -readability-magic-numbers,
  -cppcoreguidelines-pro-type-reinterpret-cast,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic

WarningsAsErrors: ''

HeaderFilterRegex: 'src/.*'

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelCase
  - key: readability-identifier-naming.VariableCase
    value: camelCase
  - key: readability-identifier-naming.PrivateMemberPrefix
    value: m_
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
  - key: readability-identifier-naming.EnumConstantCase
    value: CamelCase
  - key: readability-function-cognitive-complexity.Threshold
    value: 25
  - key: cppcoreguidelines-special-member-functions.AllowSoleDefaultDtor
    value: true
```

### .gitignore

```gitignore
# Build directories
build/
cmake-build-*/
out/

# IDE
.idea/
.vscode/
*.swp
*.swo
*~

# Compiled
*.o
*.a
*.so
*.dylib

# Qt
*.pro.user*
*.qmlc
*.jsc
moc_*
ui_*
qrc_*

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
compile_commands.json

# Testing
Testing/

# OS
.DS_Store
Thumbs.db

# Coverage
*.gcno
*.gcda
*.gcov
coverage/
```

### .devcontainer/devcontainer.json

```json
{
    "name": "devdash",
    "build": {
        "dockerfile": "Dockerfile"
    },
    "runArgs": [
        "--privileged",
        "--cap-add=NET_ADMIN"
    ],
    "customizations": {
        "vscode": {
            "extensions": [
                "llvm-vs-code-extensions.vscode-clangd",
                "ms-vscode.cmake-tools",
                "twxs.cmake",
                "ms-vscode.cpptools"
            ],
            "settings": {
                "cmake.configureOnOpen": true,
                "cmake.generator": "Ninja",
                "clangd.arguments": [
                    "--background-index",
                    "--clang-tidy",
                    "--header-insertion=iwyu",
                    "--completion-style=detailed"
                ],
                "editor.formatOnSave": true
            }
        }
    },
    "postCreateCommand": "bash .devcontainer/setup-vcan.sh",
    "remoteUser": "vscode"
}
```

### .devcontainer/Dockerfile

```dockerfile
FROM arm64v8/ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# System packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    pkg-config \
    clang \
    clang-format \
    clang-tidy \
    gdb \
    # Qt 6
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-multimedia-dev \
    qt6-serialbus-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    # GStreamer
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    # CAN
    can-utils \
    iproute2 \
    # Python (for haltech-mock)
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install haltech-mock
RUN pip3 install haltech-mock || echo "haltech-mock not yet published"

# Create non-root user
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
WORKDIR /workspace
```

### .devcontainer/setup-vcan.sh

```bash
#!/bin/bash
set -e

# Load vcan module
sudo modprobe vcan || echo "vcan module not available (expected in container)"

# Create virtual CAN interface
sudo ip link add dev vcan0 type vcan 2>/dev/null || true
sudo ip link set up vcan0 2>/dev/null || true

echo "Virtual CAN interface vcan0 configured (if supported)"
```

### .github/workflows/ci.yml

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-24.04
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            cmake ninja-build \
            clang clang-format clang-tidy \
            qt6-base-dev qt6-declarative-dev \
            qt6-multimedia-dev qt6-serialbus-dev \
            qml6-module-qtquick qml6-module-qtquick-controls
      
      - name: Configure
        run: cmake --preset ci
      
      - name: Build
        run: cmake --build build/ci
      
      - name: Test
        run: ctest --preset ci
      
      - name: Format Check
        run: cmake --build build/ci --target format-check

  lint:
    runs-on: ubuntu-24.04
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            cmake ninja-build clang clang-tidy \
            qt6-base-dev qt6-declarative-dev \
            qt6-multimedia-dev qt6-serialbus-dev
      
      - name: Configure with clang-tidy
        run: cmake --preset ci
      
      - name: Run clang-tidy
        run: |
          cmake --build build/ci 2>&1 | tee build.log
          if grep -q "warning:" build.log; then
            echo "clang-tidy warnings found"
            exit 1
          fi
```

### tests/CMakeLists.txt

```cmake
include(Catch)

# Test executable
add_executable(devdash_tests
    test_main.cpp
    core/test_data_broker.cpp
    adapters/haltech/test_haltech_protocol.cpp
)

target_link_libraries(devdash_tests PRIVATE
    devdash_core
    devdash_adapters
    Catch2::Catch2WithMain
    Qt6::Core
)

# Discover tests for CTest
catch_discover_tests(devdash_tests)
```

### tests/test_main.cpp

```cpp
// Catch2 provides main when linking Catch2::Catch2WithMain
// This file can be used for global setup if needed

#include <QCoreApplication>
#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    // Initialize Qt for tests that need it
    QCoreApplication app(argc, argv);
    
    return Catch::Session().run(argc, argv);
}
```

## Key Implementation Notes

### Testing Strategy

1. **Unit Tests (Catch2)**: Test individual classes in isolation
   - Protocol parsing
   - Data conversion
   - Configuration loading

2. **Integration Tests (Qt Test)**: Test Qt-specific behavior
   - Signal/slot connections
   - QML bindings
   - Event loop behavior

3. **System Tests**: Manual testing on hardware
   - Real CAN bus
   - Multi-display
   - Performance

### Linting Philosophy

The clang-tidy config is intentionally strict but not annoying:

- **Enabled**: Most `cppcoreguidelines-*`, `bugprone-*`, `performance-*`
- **Disabled**: 
  - `modernize-use-trailing-return-type` (personal preference)
  - `readability-identifier-length` (too noisy for loop variables)
  - `cppcoreguidelines-avoid-magic-numbers` (sometimes constants are clearer inline)
  - `cppcoreguidelines-pro-type-reinterpret-cast` (needed for CAN byte manipulation)

### Why These Choices

| Tool | Choice | Why |
|------|--------|-----|
| Build | CMake 3.25+ | Presets, FetchContent, modern features |
| C++ Standard | C++20 | Concepts, ranges, format, span |
| Test Framework | Catch2 v3 | Modern, expressive, single binary |
| Formatter | clang-format | Standard, IDE integration |
| Linter | clang-tidy | Comprehensive, cppcoreguidelines |
| Sanitizers | ASan + UBSan | Catch memory bugs early |
| Package Manager | FetchContent | Simple, no external deps |

### SOLID in Practice

The codebase demonstrates SOLID through:

1. **IProtocolAdapter interface**: All adapters implement same interface
2. **DataBroker**: Single responsibility - data aggregation only
3. **JSON profiles**: Configuration separated from code
4. **Factory pattern**: `ProtocolAdapterFactory` creates adapters
5. **Dependency injection**: Adapters injected into DataBroker

## Getting Started Command Sequence

```bash
# 1. Create repo
mkdir devdash && cd devdash
git init

# 2. Create directory structure (or use this prompt to generate)

# 3. Configure and build
cmake --preset dev
cmake --build build/dev

# 4. Run tests
ctest --preset dev

# 5. Format code
cmake --build build/dev --target format-fix

# 6. Open in VS Code with Dev Container
code .
# Then: Cmd+Shift+P → "Dev Containers: Reopen in Container"
```
