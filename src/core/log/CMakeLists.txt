project(step_core_log)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS *.hpp)
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES *.cpp)
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::core_log ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    FILE_SET headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS}"
)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    fmt::fmt
    spdlog::spdlog
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="CORE_LOG"
    GIT_INFO="${GIT_INFO}"
)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
