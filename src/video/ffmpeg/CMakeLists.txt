find_package(ffmpeg COMPONENTS avcodec avformat avfilter avdevice swscale swresample REQUIRED)

add_subdirectory(interfaces)
add_subdirectory(utils)
add_subdirectory(decoding)
add_subdirectory(reader)