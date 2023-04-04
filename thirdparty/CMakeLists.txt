add_subdirectory(easy_profiler EXCLUDE_FROM_ALL)

# stl is special as it is a header-only library:
#add_library(stl INTERFACE)
#add_library(thirdparty::stl ALIAS stl)
#target_include_directories(stl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stl_reader)

# robin_hood is not built as standalone (required to for the library)
set(RH_STANDALONE_PROJECT OFF)
add_subdirectory(robin-hood-hashing)
add_library(thirdparty::robin_hood ALIAS robin_hood)
