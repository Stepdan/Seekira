project(step_gui_ui)
set(PROJECT_ALIAS gui_ui)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_BASE *.hpp)
    file(GLOB HEADERS_CONTROLLERS controllers/*.hpp)
    file(GLOB HEADERS_VIDEO video/*.hpp)
    set(HEADERS ${HEADERS_BASE} ${HEADERS_CONTROLLERS} ${HEADERS_VIDEO})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        *.cpp
        controllers/*.cpp
        video/*.cpp
    )
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::gui_ui ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
    FILE_SET headers_controllers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/controllers FILES "${HEADERS_CONTROLLERS}"
    FILE_SET headers_video TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/video FILES "${HEADERS_VIDEO}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    step::ff_reader
    step::gui_application
    step::gui_resources
    step::proc_detect
    step::proc_drawer
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="STEP_GUI_UI"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
