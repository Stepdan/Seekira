project(Seekira)

add_executable(Seekira main.cpp)
target_link_libraries(Seekira
    PRIVATE
    step::gui_ui
)

target_compile_definitions(Seekira
    PRIVATE
    STEPKIT_MODULE_NAME="SEEKIRA"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

if (WIN32)
    windeployqt(Seekira)
endif ()

install(TARGETS Seekira RUNTIME DESTINATION ${STEPKIT_BUILD_BIN_DIR})