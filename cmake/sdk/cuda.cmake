set(CUDA_TOOLKIT_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.1")

find_package(CUDA)

if(CUDA_FOUND)
	message(STATUS "Found CUDA")
else()
	message(FATAL_ERROR "Can't found CUDA")
endif()

# CUDA requirements
# cublas64_11.dll
# cudart64_110.dll
# cufft64_10.dll

# cuDNN requirements
# cudnn_cnn_infer64_8.dll
# cudnn_ops_infer64_8.dll
# cudnn64_8.dll