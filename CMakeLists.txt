cmake_minimum_required(VERSION 3.20)

# Set project data
set(PROJECT_NAME "I2C Host Firmware")
set(BUILD_NUMBER "8")

# Set app name(s) and version(s)
set(FW_1_NAME "firmware_qtpy_rp2040")
set(FW_1_VERSION_NUMBER "0.1.2")
set(FW_1_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/qtpy")

set(FW_2_NAME "firmware_pico")
set(FW_2_VERSION_NUMBER "0.1.2")
set(FW_2_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/pico")

# Move common source code to a separate directory
set(COMMON_CODE_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/common")

# Set app-side debugging. The following defines can be set by
# uncommenting their lines:
#   BUILD_FOR_TERMINAL_TESTING -- Test firmware interaction via a terminal.
#   DO_DEBUG                   -- Put the firmware in debug mode. Shows info
#                                 on a connected LED segment display.
#   DEBUG_SEG_ADDR             -- The 7-bit I2C address of an LED segment
#                                 display used for debugging firmware
#                                 reads and writes.
#                                 IGNORED if DO_DEBUG is not defined.
#   SHOW_HEARTBEAT             -- Display a heartbeat on the board LED/NeoPixel.
#add_compile_definitions(BUILD_FOR_TERMINAL_TESTING=1)
#add_compile_definitions(DO_DEBUG=1)
add_compile_definitions(DEBUG_SEG_ADDR=0x70)
add_compile_definitions(SHOW_HEARTBEAT=1)

# Set env variable 'PICO_SDK_PATH' to the local Pico SDK
# Comment out the set() if you have a global copy of the
# SDK set and $PICO_SDK_PATH defined in your $PATH
# set(ENV{PICO_SDK_PATH} "${CMAKE_SOURCE_DIR}/pico-sdk")

# Set flags and directory variables
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DTSDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTSDEBUG")

# Include the Pico SDK
include(pico_sdk_import.cmake)

# Name the project
project(${PROJECT_NAME}
        LANGUAGES C CXX ASM
        VERSION 0.1.0
        DESCRIPTION "RP2040-based I2C host"
        HOMEPAGE_URL https://smittytone.net/cli2c
)

# Initialise the Pico SDK
pico_sdk_init()

# Include the apps' source code
add_subdirectory(${FW_1_SRC_DIRECTORY})
add_subdirectory(${FW_2_SRC_DIRECTORY})