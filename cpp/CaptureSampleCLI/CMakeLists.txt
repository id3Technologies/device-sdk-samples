cmake_minimum_required(VERSION 3.13)
set(SAMPLE_NAME CaptureSampleCLI)
project(${SAMPLE_NAME})
option(LOOP_MODE_THREAD "loop mode thread")
option(NO_CALLBACK "no callback mode")

message("LOOP_MODE_THREAD: " ${LOOP_MODE_THREAD})
message("NO_CALLBACK: " ${NO_CALLBACK})
set(REALSENSE_DEPENDENCIES)

if (LOOP_MODE_THREAD)
    add_definitions(-DLOOP_MODE_THREAD)
endif ()

set(SAMPLE_INCLUDE_DIRS
        ../../sdk/include
        )

if (NO_CALLBACK)
    set(SAMPLE_SRC_FILES
            CaptureSampleCLI_NoCallback.cpp
    )
else()
    set(SAMPLE_SRC_FILES
            CaptureSampleCLI.cpp
    )
endif()

set(CMAKE_CXX_STANDARD 20)
add_executable(${SAMPLE_NAME} ${SAMPLE_SRC_FILES})

target_include_directories(${SAMPLE_NAME} PRIVATE ${SAMPLE_INCLUDE_DIRS})
target_link_directories(${SAMPLE_NAME} PRIVATE ../../sdk/bin/windows/x64)
target_link_libraries(${SAMPLE_NAME} id3Devices)

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

postBuildCopyIfDifferent("${CMAKE_SOURCE_DIR}/../../sdk/bin/windows/x64/id3Devices.dll" "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
file(GLOB BIN_FILES "${CMAKE_SOURCE_DIR}/../../sdk/devices/Webcam/windows/x64/*.dll")
postBuildCopyIfDifferent(${BIN_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
