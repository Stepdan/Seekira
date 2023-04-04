project(steptech_base)
set(PROJECT_ALIAS base)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_TYPES types/*.hpp)
    file(GLOB HEADERS_UTILS utils/*.hpp)
    set(HEADERS ${HEADERS_TYPES} ${HEADERS_UTILS})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        types/*.cpp
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
    FILE_SET headers_types TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/types FILES "${HEADERS_TYPES}"
    FILE_SET headers_utils TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils FILES "${HEADERS_UTILS}"
)

find_package(Boost REQUIRED COMPONENTS exception stacktrace_backtrace)
find_package(Poco REQUIRED COMPONENTS Data Net Util)
find_package(Eigen3 3.3 REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Boost::stacktrace_backtrace
    Boost::exception
    Poco::Data
    Poco::Net
    Poco::Util
    Eigen3::Eigen
    step::log
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    RVISION_MODULE_NAME="RVISION_CORE"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
    COMPONENT ${PROJECT_NAME}
    FILE_SET headers_types DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/types
    FILE_SET headers_utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
