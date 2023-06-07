project(step_proc_detect)

if(NOT DEFINED HEADERS)
    file(GLOB HEADERS_BASE *.hpp)
    file(GLOB HEADERS_FASE face/*.hpp)
    set(HEADERS ${HEADERS_BASE} ${HEADERS_FASE})
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        face/*.cpp
        #*.cpp
    ) 
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::proc_detect ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
    FILE_SET headers_fase TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/face FILES "${HEADERS_FASE}"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    step::thirdparty_tdv
    step::proc_interfaces
    step::proc_settings
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="PROC_DETECT"
    PUBLIC
    BUILD_WITH_EASY_PROFILER

)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
