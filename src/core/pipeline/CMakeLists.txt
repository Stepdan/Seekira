project(step_core_pipeline)

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
add_library(step::core_pipeline ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
    FILE_SET headers_nodes TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/nodes FILES "${HEADERS_NODES}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    step::core_graph
    step::core_task
    step::core_threading
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="CORE_PIPELINE"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     FILE_SET headers_nodes DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/nodes
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
