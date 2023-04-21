project(frame_pipeline)
set(PROJECT_ALIAS ${PROJECT_NAME})

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_BASE *.hpp)
    file(GLOB HEADERS_NODES nodes/*.hpp)
    set(HEADERS ${HEADERS_BASE} ${HEADERS_NODES})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        *.cpp
        nodes/*.cpp
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
    FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
    FILE_SET headers_nodes TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/nodes FILES "${HEADERS_NODES}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    step::frame_interface
    step::abstract_task
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="FRAME_PIPELINE"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
    COMPONENT ${PROJECT_NAME}
    FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
    FILE_SET headers_nodes DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/nodes
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
