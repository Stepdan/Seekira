set(ONNXRUNTIME_ROOT "C:/Work/StepTech/SDK/onnxruntime-win-x64-gpu-1.10.0")

find_path(ONNXRUNTIME_INCLUDE_DIR NAMES onnxruntime_cxx_api.h HINTS ${ONNXRUNTIME_ROOT} PATH_SUFFIXES include)

find_library(ONNXRUNTIME_LIB NAMES onnxruntime HINTS ${ONNXRUNTIME_ROOT} PATH_SUFFIXES lib)
find_library(ONNXRUNTIME_PROVIDERS_LIB NAMES onnxruntime_providers_shared HINTS ${ONNXRUNTIME_ROOT} PATH_SUFFIXES lib)
find_library(ONNXRUNTIME_PROVIDERS_CUDA_LIB NAMES onnxruntime_providers_cuda HINTS ${ONNXRUNTIME_ROOT} PATH_SUFFIXES lib)
find_library(ONNXRUNTIME_PROVIDERS_TENSORRT_LIB NAMES onnxruntime_providers_tensorrt HINTS ${ONNXRUNTIME_ROOT} PATH_SUFFIXES lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRUNTIME DEFAULT_MSG ONNXRUNTIME_INCLUDE_DIR ONNXRUNTIME_LIB ONNXRUNTIME_PROVIDERS_LIB ONNXRUNTIME_PROVIDERS_CUDA_LIB ONNXRUNTIME_PROVIDERS_TENSORRT_LIB)
mark_as_advanced(ONNXRUNTIME_INCLUDE_DIR ONNXRUNTIME_LIB ONNXRUNTIME_PROVIDERS_LIB ONNXRUNTIME_PROVIDERS_CUDA_LIB ONNXRUNTIME_PROVIDERS_TENSORRT_LIB)

if(ONNXRUNTIME_FOUND)
	message(STATUS "Found onnxruntime")
else()
	message(FATAL_ERROR "Can't found onnxruntime")
endif()


function(link_onnxruntime)
	# Linking thirdparty libraries
	if(ONNXRUNTIME_FOUND)
		target_include_directories(${PROJECT_NAME} PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
		target_link_libraries(${PROJECT_NAME} PUBLIC ${ONNXRUNTIME_LIB})
		target_link_libraries(${PROJECT_NAME} PUBLIC ${ONNXRUNTIME_PROVIDERS_LIB})
		target_link_libraries(${PROJECT_NAME} PUBLIC ${ONNXRUNTIME_PROVIDERS_CUDA_LIB})
		target_link_libraries(${PROJECT_NAME} PUBLIC ${ONNXRUNTIME_PROVIDERS_TENSORRT_LIB})
		message(STATUS "Link onnxruntime to ${PROJECT_NAME}")
	else()
		message(FATAL_ERROR "Can't link onnxruntime to ${PROJECT_NAME}")
	endif()
endfunction()