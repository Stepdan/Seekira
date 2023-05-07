project(step_ui_resources)

add_library(${PROJECT_NAME} STATIC
    ./resources.qrc
    )

add_library(step::ui_resources ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "")

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="STEP_UI_RESOURCES"
)