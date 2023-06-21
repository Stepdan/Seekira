set(OpenVINO_DIR "C:/Work/StepTech/SDK/w_openvino_toolkit_windows_2023.0.0.10926.b4452d56304_x86_64/runtime/cmake")

find_package(OpenVINO)

if(OpenVINO_FOUND)
	message(STATUS "Found OpenVINO")
else()
	message(FATAL_ERROR "Can't found OpenVINO")
endif()