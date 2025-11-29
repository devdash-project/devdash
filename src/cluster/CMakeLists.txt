# Cluster window module

qt_add_qml_module(devdash_cluster
    URI DevDash.Cluster
    VERSION 1.0
    SOURCES
        ClusterWindow.cpp
        ClusterWindow.h
    QML_FILES
        qml/ClusterMain.qml
        qml/gauges/Tachometer.qml
    RESOURCE_PREFIX /
)

target_include_directories(devdash_cluster PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(devdash_cluster PUBLIC
    devdash_core
    Qt6::Quick
    Qt6::Qml
)

set_project_warnings(devdash_cluster)
enable_sanitizers(devdash_cluster)
# Skip clang-tidy for QML modules (qt_add_qml_module generates .rcc files with magic numbers)
# enable_clang_tidy(devdash_cluster)
