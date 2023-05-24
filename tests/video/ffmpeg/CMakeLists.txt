project(step_tests_ff)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    #gmock
    gtest
    gtest_main
    step::ff_reader
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FF_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
    STEPKIT_MODULE_NAME="T_FF"
)

gtest_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${CMAKE_CURRENT_BINARY_DIR})
