add_subdirectory(thirdparty)

add_subdirectory(core)
add_subdirectory(video)
add_subdirectory(proc)
add_subdirectory(application)

if(WITH_GUI)
add_subdirectory(gui)
endif()