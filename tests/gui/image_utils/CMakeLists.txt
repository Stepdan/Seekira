project(step_tests_gui_image_utils)

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
    step::gui_utils
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    GUI_IMAGE_UTILS_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/"
    STEPKIT_MODULE_NAME="T_GUI_IMAGE_UTILS"
)

if (WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif ()

gtest_discover_tests(${PROJECT_NAME} WORKING_DIRECTORY ${STEPKIT_BUILD_BIN_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${STEPKIT_BUILD_BIN_DIR})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${STEPKIT_BUILD_BIN_DIR})