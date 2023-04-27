project(camera_fake)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS *.hpp)
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES *.cpp)
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC
    ${HEADERS}
    ${SOURCES}
)
add_library(step::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        step::camera_interfaces
        step::frame_utils
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        STEPKIT_MODULE_NAME="CAMERA_FAKE"
    PUBLIC
        BUILD_WITH_EASY_PROFILER
)
