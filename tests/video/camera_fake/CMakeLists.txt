project(camera_fake_tests)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS *.hpp)
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES *.cpp)
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    #gmock
    gtest
    gtest_main
    step::camera_fake
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    CAMERA_FAKE_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
    STEPKIT_MODULE_NAME="CAMERA_FRAME_TESTS"
)

gtest_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${CMAKE_CURRENT_BINARY_DIR})
