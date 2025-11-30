# Core library - DataBroker and interfaces

add_library(devdash_core STATIC
    broker/DataBroker.cpp
    broker/DataBroker.h
    channels/ChannelTypes.h
    channels/ChannelUpdateQueue.cpp
    channels/ChannelUpdateQueue.h
    conversion/DefaultUnitConverter.cpp
    conversion/DefaultUnitConverter.h
    devtools/DevToolsServer.cpp
    devtools/DevToolsServer.h
    interfaces/IDataSource.h
    interfaces/IProtocolAdapter.h
    interfaces/IUnitConverter.h
    logging/LogCategories.cpp
    logging/LogCategories.h
    logging/LogManager.cpp
    logging/LogManager.h
)

target_include_directories(devdash_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${concurrentqueue_SOURCE_DIR}
)

target_link_libraries(devdash_core PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Quick
    nlohmann_json::nlohmann_json
)

set_project_warnings(devdash_core)
enable_sanitizers(devdash_core)
enable_clang_tidy(devdash_core)
