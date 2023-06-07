cmake_minimum_required(VERSION 3.25)
project(stepkit)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_compile_definitions(STEPKIT_DEBUG)
    message(STATUS "STEPKIT Debug build")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF) # the flag used to set the language standard (e.g. -std=c++11 rather than -std=gnu++11).
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_GENERATOR "Ninja") 

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdk)

set(STEPKIT_INSTALL_DIR .)
set(STEPKIT_INSTALL_BIN_DIR bin)
set(STEPKIT_INSTALL_LIB_DIR lib)
set(STEPKIT_BUILD_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(STEPKIT_BUILD_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${STEPKIT_BUILD_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${STEPKIT_BUILD_LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${STEPKIT_BUILD_BIN_DIR})

set(WITH_GUI true)

if(WITH_GUI)
    # Qt configuration
    set(QT_DIR "C:/Qt/5.15.2/msvc2019_64")
    if(NOT QT_DIR)
        message(NOTICE "Define QT_DIR to use external Qt")
    endif()

    # Describe conan dependencies
    set(USE_EXTERNAL_QT FALSE)
    if(DEFINED QT_DIR)
        set(USE_EXTERNAL_QT TRUE)
    endif()

    if(USE_EXTERNAL_QT)
        message(NOTICE "Using Qt from ${QT_DIR}. To use conan version undef QT_DIR variable")
        file(REMOVE "${CMAKE_BINARY_DIR}/FindQt5.cmake")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${QT_DIR}/lib/cmake/Qt5/")
    else()
        message(NOTICE "Using Qt from conan. To use external Qt build define QT_DIR variable")
        set(CONAN_Qt zstd/1.5.5 freetype/2.13.0 qt/5.15.8)
        set(CONAN_OPTIONS_Qt
                qt:shared=True
                qt:commercial=False
                qt:opengl=desktop
                qt:qtdoc=False
                qt:qtdeclarative=True
                qt:qtquickcontrols=True
                qt:qtquickcontrols2=True
                qt:qtmultimedia=True
                qt:qtwebengine=False
                qt:with_zstd=True
                qt:with_pq=False
                qt:with_odbc=False
                qt:with_sqlite3=False
            )
    endif()
endif()

include(conan)

conan_cmake_run(
    REQUIRES
        fmt/9.1.0
        spdlog/1.11.0
        nlohmann_json/3.11.2
        poco/1.12.4
        boost/1.81.0
        ffmpeg/5.1
        protobuf/3.21.9
        #onnx/1.14.0 #TODO Ждем onnxruntime, для винды нет: https://conan.io/center/onnxruntime
        opencv/4.5.5
        ${CONAN_Qt}
    BASIC_SETUP
    CMAKE_TARGETS
    UPDATE
    OPTIONS
        spdlog:header_only=False
        #-----------------------------
        opencv/*:shared=True
        opencv/*:with_eigen=True
        opencv/*:with_ffmpeg=True
        opencv/*:contrib=False
        opencv/*:parallel=False
        opencv/*:with_tiff=False
        opencv/*:with_webp=False
        opencv/*:with_ade=False
        opencv/*:with_openexr=False
        opencv/*:with_webp=False
        opencv/*:with_quirc=False
        opencv/*:with_cuda=False
        opencv/*:with_cublas=False
        opencv/*:with_cufft=False
        opencv/*:with_cudnn=False
        opencv/*:with_ffmpeg=False
        #-----------------------------
        boost:shared=True
        boost:zlib=False
        boost:lzma=False
        boost:zstd=False
        boost:bzip2=False
        boost:without_date_time=False
        boost:without_exception=False
        boost:without_filesystem=False
        boost:without_program_options=False
        boost:without_stacktrace=False
        boost:without_thread=False
        boost:without_graph=True
        boost:without_iostreams=True
        boost:without_json=True
        boost:without_locale=True
        boost:without_log=True
        boost:without_math=True
        boost:without_mpi=True
        boost:without_nowide=True
        boost:without_python=True
        boost:without_random=True
        boost:without_python=True
        boost:without_regex=True
        boost:without_serialization=True
        boost:without_test=True
        boost:without_timer=True
        boost:without_type_erasure=True
        boost:without_wave=True
        #-----------------------------
        pcre:with_bzip2=False
        #-----------------------------
        poco:shared=True
        poco:enable_zip=False
        poco:enable_xml=True
        poco:enable_data_sqlite=False
        poco:enable_mongodb=False
        poco:enable_netssl=False
        poco:enable_crypto=False
        poco:enable_jwt=False
        poco:enable_data_postgresql=False
        poco:enable_data_mysql=False
        #-----------------------------
        ${CONAN_OPTIONS_Qt}
    BUILD
        missing
    GENERATORS
        cmake_paths
        cmake_find_package
    IMPORTS
        "bin, *.dll -> ${STEPKIT_BUILD_BIN_DIR}"
    )

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake)

# Qt
if(WITH_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    set(QT_VERSION 5)
    set(QT "Qt${QT_VERSION}")
    set(${QT}_DIR "${QT_DIR}/bin")

    set(QT_COMPONENTS "Core;Gui;Widgets;Qml;Quick;QuickControls2;Multimedia")
    find_package(${QT} COMPONENTS ${QT_COMPONENTS} REQUIRED)
    if(USE_EXTERNAL_QT)
        set(QML_IMPORT_PATH "${QT_DIR}/qml")
        set(QT_PLUGINS_PATH "${QT_DIR}/plugins")
        add_library(${QT}::${QT} INTERFACE IMPORTED)
        foreach(component ${QT_COMPONENTS})
            set(QT_LIBS "${QT}::${component};${QT_LIBS}")
        endforeach()
        set_property(TARGET ${QT}::${QT} PROPERTY INTERFACE_LINK_LIBRARIES ${QT_LIBS})
    else()
        message(FATAL_ERROR "TODO Set qt dirs for conan package configuration")
        #set(QT_DIR "${${QT}_Core_LIB_DIRS}/..")
        #set(QML_IMPORT_PATH "${QT_DIR}/bin/archdatadir/qml")
        #set(QT_PLUGINS_PATH "${QT_DIR}/bin/archdatadir/plugins")
    endif()
endif()

find_package(Boost REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(Poco REQUIRED)
find_package(gtest REQUIRED)
find_package(ffmpeg REQUIRED)

find_package(OpenCV REQUIRED)
include_directories ("/usr/include/opencv4/")

include(onnxruntime)

set(INSTALL_TARGET_NAME "stepkit-targets")

include_directories(thirdparty)
include_directories(src)

add_subdirectory(thirdparty)
add_subdirectory(src)

option(ENABLE_TESTS "Build tests" ON)

if(ENABLE_TESTS)
    # ensure failed tests produce visible output out of the box
    set(CMAKE_CTEST_ARGUMENTS "${CMAKE_CTEST_ARGUMENTS};--output-on-failure")
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()