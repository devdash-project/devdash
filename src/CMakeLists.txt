# Source directory - builds all modules and main executable

# Add subdirectories for each module
add_subdirectory(core)
add_subdirectory(adapters)
add_subdirectory(cluster)
add_subdirectory(headunit)

# Main executable
qt_add_executable(devdash
    main.cpp
)

# Link against base libraries (QML modules are loaded at runtime, not linked)
target_link_libraries(devdash PRIVATE
    devdash_core
    devdash_adapters
    devdash_cluster
    devdash_headunit
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
)

set_project_warnings(devdash)
enable_sanitizers(devdash)

# Place binary in build root (e.g., build/dev/devdash instead of build/dev/src/devdash)
set_target_properties(devdash PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Install rules
install(TARGETS devdash
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
