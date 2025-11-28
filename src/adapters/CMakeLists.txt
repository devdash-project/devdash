# Protocol adapters library

add_library(devdash_adapters STATIC
    ProtocolAdapterFactory.cpp
    ProtocolAdapterFactory.h
    haltech/HaltechAdapter.cpp
    haltech/HaltechAdapter.h
    haltech/HaltechProtocol.cpp
    haltech/HaltechProtocol.h
    haltech/PD16Protocol.cpp
    haltech/PD16Protocol.h
    simulator/SimulatorAdapter.cpp
    simulator/SimulatorAdapter.h
)

target_include_directories(devdash_adapters PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(devdash_adapters PUBLIC
    devdash_core
    Qt6::Core
    Qt6::SerialBus
    nlohmann_json::nlohmann_json
)

set_project_warnings(devdash_adapters)
enable_sanitizers(devdash_adapters)
enable_clang_tidy(devdash_adapters)
