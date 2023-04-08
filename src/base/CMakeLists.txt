project(steptech_base)
set(PROJECT_ALIAS base)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_BASE *.hpp)
    file(GLOB HEADERS_TYPES types/*.hpp)
    file(GLOB HEADERS_UTILS utils/*.hpp)
    file(GLOB HEADERS_UTILS_EXCEPTION utils/exception/*.hpp)
    file(GLOB HEADERS_UTILS_JSON utils/json/*.hpp)
    file(GLOB HEADERS_UTILS_THREAD utils/thread/*.hpp)
    set(HEADERS ${HEADERS_BASE} ${HEADERS_TYPES} ${HEADERS_UTILS} ${HEADERS_UTILS_EXCEPTION} ${HEADERS_UTILS_JSON} ${HEADERS_UTILS_THREAD})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        types/*.cpp
        utils/*.cpp
        utils/exception/*.cpp
        utils/json/*.cpp
        utils/thread/*.cpp
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
    FILE_SET headers_types TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/types FILES "${HEADERS_TYPES}"
    FILE_SET headers_utils TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils FILES "${HEADERS_UTILS}"
    FILE_SET headers_utils_exception TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils/exception FILES "${HEADERS_UTILS_EXCEPTION}"
    FILE_SET headers_utils_json TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils/json FILES "${HEADERS_UTILS_JSON}"
    FILE_SET headers_utils_thread TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils/thread FILES "${HEADERS_UTILS_THREAD}"
)

find_package(Boost REQUIRED COMPONENTS exception stacktrace)
find_package(Poco REQUIRED COMPONENTS Data Net Util)
find_package(Eigen3 3.3 REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Boost::stacktrace
    Boost::exception
    Poco::Data
    Poco::Net
    Poco::Util
    Eigen3::Eigen
    step::log
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="STEPKIT_BASE"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
    COMPONENT ${PROJECT_NAME}
    FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
    FILE_SET headers_types DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/types
    FILE_SET headers_utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils
    FILE_SET headers_utils_exception DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils/exception
    FILE_SET headers_utils_json DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils/json
    FILE_SET headers_utils_thread DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils/thread
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
