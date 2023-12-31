project(step_tests_exception_frame_pipeline)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(${PROJECT_NAME} PRIVATE
    #gmock
    gtest
    gtest_main
    step::frame_utils
    step::proc_pipeline
    step::proc_interfaces
    step::application
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    EXCPETION_FRAME_PIPELINE_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
    STEPKIT_MODULE_NAME="T_EXCEPTION_PIPELINE"
)

gtest_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY ${STEPKIT_BUILD_BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${STEPKIT_BUILD_BIN_DIR})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${STEPKIT_BUILD_BIN_DIR})