project(step_gui)
set(PROJECT_ALIAS gui)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_UTILS utils/*.hpp)
    set(HEADERS ${HEADERS_UTILS})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        utils/*.cpp
    )
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::${PROJECT_ALIAS} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    FILE_SET headers_utils TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils FILES "${HEADERS_UTILS}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Qt5::Qt5
    step::frame_utils
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="STEPKIT_GUI"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
    COMPONENT ${PROJECT_NAME}
    FILE_SET headers_utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_executable(step_app app/main.cpp)
target_link_libraries(step_app
    PRIVATE
    step::gui
)

if (WIN32)
    add_custom_command(
        TARGET step_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:step_app> $<TARGET_FILE_DIR:step_app>
        COMMAND_EXPAND_LISTS
    )
endif ()

install(TARGETS step_app RUNTIME DESTINATION ${STEPKIT_BUILD_BIN_DIR})