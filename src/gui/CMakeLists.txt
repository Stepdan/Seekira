add_subdirectory(utils)
add_subdirectory(interfaces)
add_subdirectory(ui)
add_subdirectory(resources)

if(WIN32)
    set(QML_FILES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/qml)
    include(windeployqt)
endif()

add_subdirectory(app)