project(step_core_base)

if(NOT DEFINED HEADERS)
    #file(GLOB HEADERS_BASE *.hpp)
    file(GLOB HEADERS_TYPES types/*.hpp)
    file(GLOB HEADERS_UTILS utils/*.hpp)
    file(GLOB HEADERS_JSON json/*.hpp)
    file(GLOB HEADERS_INTERFACES interfaces/*.hpp)
    set(HEADERS ${HEADERS_TYPES} ${HEADERS_UTILS} ${HEADERS_JSON} ${HEADERS_INTERFACES}) # ${HEADERS_BASE}
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        types/*.cpp
        interfaces/*.cpp
        utils/*.cpp
        json/*.cpp
    )
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::core_base ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    #FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
    FILE_SET headers_types TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/types FILES "${HEADERS_TYPES}"
    FILE_SET headers_utils TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils FILES "${HEADERS_UTILS}"
    FILE_SET headers_json TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/json FILES "${HEADERS_JSON}"
    FILE_SET headers_interfaces TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/interfaces FILES "${HEADERS_INTERFACES}"
)

find_package(Poco REQUIRED COMPONENTS Data Net Util)
find_package(Eigen3 3.3 REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Poco::Data
    Poco::Net
    Poco::Util
    Eigen3::Eigen
    step::core_exception
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="CORE_BASE"
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
