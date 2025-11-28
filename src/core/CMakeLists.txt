# Core library - DataBroker and interfaces

add_library(devdash_core STATIC
    DataBroker.cpp
    DataBroker.h
    IProtocolAdapter.h
)

target_include_directories(devdash_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(devdash_core PUBLIC
    Qt6::Core
    nlohmann_json::nlohmann_json
)

set_project_warnings(devdash_core)
enable_sanitizers(devdash_core)
enable_clang_tidy(devdash_core)
