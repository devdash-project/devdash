# Head unit window module

qt_add_qml_module(devdash_headunit
    URI DevDash.HeadUnit
    VERSION 1.0
    SOURCES
        HeadUnitWindow.cpp
        HeadUnitWindow.h
    QML_FILES
        qml/HeadUnitMain.qml
    RESOURCE_PREFIX /
)

target_include_directories(devdash_headunit PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(devdash_headunit PUBLIC
    devdash_core
    Qt6::Quick
    Qt6::Qml
    Qt6::Multimedia
)

set_project_warnings(devdash_headunit)
enable_sanitizers(devdash_headunit)
# Skip clang-tidy for QML modules (qt_add_qml_module generates .rcc files with magic numbers)
# enable_clang_tidy(devdash_headunit)
