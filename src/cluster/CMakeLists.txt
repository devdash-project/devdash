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

# QML linting with qmllint (force Qt6 version, avoid broken system wrapper)
if(EXISTS /usr/lib/qt6/bin/qmllint)
    set(QMLLINT_EXECUTABLE /usr/lib/qt6/bin/qmllint)
else()
    find_program(QMLLINT_EXECUTABLE NAMES qmllint6 HINTS ${Qt6_DIR}/../../../bin)
endif()

if(QMLLINT_EXECUTABLE)
    set(QML_FILES_TO_LINT
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/ClusterMain.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/Tachometer.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeArc.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeNeedle.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeFace.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeTick.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeTickLabel.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeCenterCap.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/gauges/primitives/GaugeBezel.qml
    )

    add_custom_target(qmllint-cluster
        COMMAND ${QMLLINT_EXECUTABLE} ${QML_FILES_TO_LINT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running qmllint on cluster QML files"
        VERBATIM
    )

    message(STATUS "qmllint found: ${QMLLINT_EXECUTABLE}")
    message(STATUS "Run 'cmake --build build/dev --target qmllint-cluster' to lint QML files")
else()
    message(WARNING "qmllint not found - QML linting disabled")
endif()
