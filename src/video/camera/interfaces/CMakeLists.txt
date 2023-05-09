project(step_camera_interfaces)
set(PROJECT_ALIAS camera_interfaces)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_TYPES types/*.hpp)
    file(GLOB HEADERS_ROOT *.hpp)
    set(HEADERS ${HEADERS_ROOT} ${HEADERS_TYPES})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        types/*.cpp
        *.cpp
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
    FILE_SET headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS}"
    FILE_SET headers_types TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/types FILES "${HEADERS_TYPES}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    step::frame_interface
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="CAMERA_INTERFACES"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     FILE_SET headers_types DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/types
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
