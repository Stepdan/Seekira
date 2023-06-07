project(step_face_sdk_1)

if(NOT DEFINED HEADERS)
    #file(GLOB HEADERS_BASE *.hpp)
    file(GLOB HEADERS_API api/*.h)
    file(GLOB HEADERS_DATA data/*.h)
    file(GLOB HEADERS_MODULES modules/*.h)
    file(GLOB HEADERS_DETECTION_MODULES modules/detection_modules/*.h)
    file(GLOB HEADERS_RASSERT utils/rassert/*.h)
    file(GLOB HEADERS_REC_UTILS utils/recognizer_utils/*.h)
    set(HEADERS ${HEADERS_API} ${HEADERS_DATA} ${HEADERS_MODULES} ${HEADERS_DETECTION_MODULES} ${HEADERS_RASSERT} ${HEADERS_REC_UTILS}) # ${HEADERS_BASE}
endif()

if(NOT DEFINED SOURCES)
    file(GLOB SOURCES
        api/*.cpp
        data/*.cpp
        modules/*.cpp
        modules/detection_modules/*.cpp
        utils/recognizer_utils/*.cpp
    )
endif()

source_group("Headers" FILES ${HEADERS})
source_group("Sources" FILES ${SOURCES})

add_library(${PROJECT_NAME} STATIC)
add_library(step::thirdparty_tdv ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
    ${SOURCES}
    PUBLIC
    #FILE_SET headers_base TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES "${HEADERS_BASE}"
    FILE_SET headers_api TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/api FILES "${HEADERS_API}"
    FILE_SET headers_data TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/data FILES "${HEADERS_DATA}"
    FILE_SET headers_modules TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules FILES "${HEADERS_MODULES}"
    FILE_SET headers_detection_modules TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules/detection_modules FILES "${HEADERS_DETECTION_MODULES}"
    FILE_SET headers_rassert TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils/rassert FILES "${HEADERS_RASSERT}"
    FILE_SET headers_rec_utils TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/utils/recognizer_utils FILES "${HEADERS_REC_UTILS}"
)

find_package(OpenCV REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
    nlohmann_json::nlohmann_json
)

link_onnxruntime()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    STEPKIT_MODULE_NAME="STEP_FACE_SDK_1"
    PUBLIC
    BUILD_WITH_EASY_PROFILER
)

# install(TARGETS ${PROJECT_NAME} EXPORT ${INSTALL_TARGET_NAME}
#     COMPONENT ${PROJECT_NAME}
#     FILE_SET headers_base DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}
#     FILE_SET headers_types DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/types
#     FILE_SET headers_utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/utils
#     FILE_SET headers_json DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/json
#     FILE_SET headers_interfaces DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_ALIAS}/interfaces
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )
