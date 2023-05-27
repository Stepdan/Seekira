project(step_app)

add_executable(step_app main.cpp)
target_link_libraries(step_app
    PRIVATE
    step::gui_ui
)

target_compile_definitions(step_app
    PRIVATE
    STEPKIT_MODULE_NAME="STEP_APP"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

if (WIN32)
    windeployqt(step_app)
endif ()

install(TARGETS step_app RUNTIME DESTINATION ${STEPKIT_BUILD_BIN_DIR})