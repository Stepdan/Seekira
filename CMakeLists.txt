cmake_minimum_required(VERSION 3.25)
project(stepkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF) # the flag used to set the language standard (e.g. -std=c++11 rather than -std=gnu++11).
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_GENERATOR_TOOLSET "v143")

set(STEPKIT_INSTALL_DIR .)
set(STEPKIT_INSTALL_BIN_DIR bin)
set(STEPKIT_INSTALL_LIB_DIR lib)
set(STEPKIT_BUILD_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(STEPKIT_BUILD_LIB_DIR ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(conan)

conan_cmake_run(
    REQUIRES
        fmt/9.1.0
        spdlog/1.11.0
        eigen/3.4.0
        jasper/2.0.32        # opencv
        jsoncpp/1.9.5        # opencv
        libjpeg/9e           # opencv
        libpng/1.6.37        # opencv
        opencv/4.5.5
        boost/1.81.0
        poco/1.12.4
        libbacktrace/cci.20210118
    BASIC_SETUP
    CMAKE_TARGETS
    UPDATE
    OPTIONS
        spdlog:header_only=True
        #-----------------------------
        opencv/*:shared=True
        #opencv/*:fPIC=True
        opencv/*:contrib=True
        opencv/*:with_eigen=True
        opencv/*:parallel=False
        opencv/*:with_tiff=False
        opencv/*:with_webp=False
        opencv/*:with_ffmpeg=False
        opencv/*:with_ade=False
        #opencv/*:with_imath=False
        opencv/*:with_openexr=False
        opencv/*:with_webp=False
        opencv/*:with_quirc=False
        #-----------------------------
        boost:shared=False
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
    BUILD
        missing
    GENERATORS
        cmake_paths
        cmake_find_package
    IMPORTS
    )

include(${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake)

find_package(Eigen3 REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(robin_hood REQUIRED)
find_package(Poco REQUIRED)
find_package(gtest REQUIRED)
#find_package(easy_profiler REQUIRED)

find_package(OpenCV REQUIRED)
include_directories ("/usr/include/opencv4/")

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

install(DIRECTORY ${STEPKIT_INSTALL_LIB_DIR}
    DESTINATION ${STEPKIT_INSTALL_DIR})