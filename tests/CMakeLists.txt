project(stepkit_tests)

# enable test infrastructure of CMake:
add_subdirectory(googletest)
mark_as_advanced(BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols)

# test suites:
#add_subdirectory(core)
add_subdirectory(video)
add_subdirectory(proc)
#add_subdirectory(serializable)
#add_subdirectory(easy_profiler)

if(WITH_GUI)
    add_subdirectory(gui)
endif()