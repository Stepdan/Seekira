project(step_face_engine_openvino)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_BASE *.hpp)
    set(HEADERS ${HEADERS_BASE})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        *.cpp
    )
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::face_engine_openvino ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    
    step::frame_utils
    step::proc_interfaces
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="PROC_FACE_ENGINE_OPENVINO"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     FILE_SET headers_types DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/types
#     FILE_SET headers_utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils
#     FILE_SET headers_json DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/json
#     FILE_SET headers_interfaces DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/interfaces
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
