cmake_minimum_required(VERSION 3.13)
set(SAMPLE_NAME CaptureSampleOpenCv)
project(${SAMPLE_NAME})
option(PLUG_AND_PLAY "Plug and play")

message("PLUG_AND_PLAY: " ${PLUG_AND_PLAY})
set(REALSENSE_DEPENDENCIES)

set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/../../opencv/windows_static")
set(OpenCV_INCLUDE_DIRS "${OpenCV_DIR}/include")
set(OPENCV_MANUAL_LIB_DIR "${OpenCV_DIR}/x64/vc15/lib")
set(OPENCV_MANUAL_BIN_DIR "${OpenCV_DIR}/x64/vc15/bin")
set(OpenCV_LIB opencv_world460d.lib)
set(OpenCV_DLL opencv_world460d.dll)

set(SAMPLE_INCLUDE_DIRS
        ../../sdk/include
        ${OpenCV_INCLUDE_DIRS}
        )

if (PLUG_AND_PLAY)
set(SAMPLE_SRC_FILES
        CaptureSampleOpenCvPP.cpp
        )
else ()
set(SAMPLE_SRC_FILES
        CaptureSampleOpenCv.cpp
        )
endif()

set(CMAKE_CXX_STANDARD 17)
add_executable(${SAMPLE_NAME} ${SAMPLE_SRC_FILES})

target_include_directories(${SAMPLE_NAME} PRIVATE ${SAMPLE_INCLUDE_DIRS})
target_link_directories(${SAMPLE_NAME} PRIVATE ../../sdk/bin/windows/x64 ${OPENCV_MANUAL_LIB_DIR})
target_link_libraries(${SAMPLE_NAME} id3Devices ${OpenCV_LIB})

# Post-build copy if different
# Args:
# - target: target which build will trigger the copy
# - dst_dir: the destination directory of the copy
# - argn: the list of files to copy if different
macro(postBuildCopyIfDifferent file_src dst_dir)
    add_custom_command(
            TARGET ${SAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${file_src}
            ${dst_dir})
endmacro()

# devices
postBuildCopyIfDifferent("${CMAKE_SOURCE_DIR}/../../sdk/bin/windows/x64/id3Devices.dll" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

# webcam
file(GLOB BIN_FILES "${CMAKE_SOURCE_DIR}/../../sdk/devices/Webcam/windows/x64/*.dll")
postBuildCopyIfDifferent(${BIN_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")

# opencv
postBuildCopyIfDifferent("${OPENCV_MANUAL_BIN_DIR}/${OpenCV_DLL}" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
