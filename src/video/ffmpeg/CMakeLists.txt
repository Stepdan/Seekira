find_package(ffmpeg COMPONENTS avcodec avformat avfilter avdevice swscale swresample REQUIRED)

add_subdirectory(utils)
add_subdirectory(decoding)