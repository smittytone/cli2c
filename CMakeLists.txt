cmake_minimum_required(VERSION 3.18)

# Set project data
set(PROJECT_NAME "I2C Host Firmware")
set(FW_BUILD_NUMBER "34")
set(FW_VERSION_NUMBER "1.1.3")

# Set app name(s) and version(s)
set(FW_1_NAME "firmware_qtpy_rp2040")
set(FW_1_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/qtpy")

set(FW_2_NAME "firmware_pico")
set(FW_2_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/pico")

set(FW_3_NAME "firmware_promicro")
set(FW_3_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/promicro")

set(FW_4_NAME "firmware_tiny2040")
set(FW_4_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/tiny")

set(FW_5_NAME "firmware_trinkey2040")
set(FW_5_SRC_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/trinkey")

# Move common source code to a separate directory
set(COMMON_CODE_DIRECTORY "${CMAKE_SOURCE_DIR}/firmware/common")

# Set app-side debugging. The following defines can be set by
# uncommenting their lines:
#   DO_UART_DEBUG              -- Put the firmware in debug mode.
#   DEBUG_SEG_ADDR             -- The 7-bit I2C address of an LED segment
#                                 display used for debugging firmware
#                                 reads and writes.
#                                 IGNORED if DO_DEBUG is not defined.
#   SHOW_HEARTBEAT             -- Display a heartbeat on the board LED/NeoPixel.
set(DO_DEBUG 1)
if(DO_DEBUG)
add_compile_definitions(DO_UART_DEBUG=1)
add_compile_definitions(DEBUG_SEG_ADDR=0x70)
endif()

# Show the bus host's pulse
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

# FROM 1.1.3
# Recommend usage of SDK 1.5.0+
if(PICO_SDK_VERSION_MINOR LESS "5")
message(ERROR " WE STRONGLY RECOMMEND YOU BUILD THE FIRMWARE WITH PICO SDK 1.5.0 OR ABOVE")
endif()

# Name the project
project(${PROJECT_NAME}
        LANGUAGES C CXX ASM
        VERSION ${FW_VERSION_NUMBER}
        DESCRIPTION "RP2040-based I2C host"
        HOMEPAGE_URL https://smittytone.net/cli2c
)

# Initialise the Pico SDK
pico_sdk_init()

# Include the apps' source code
# Comment out any that you don't require (speeds build time)
add_subdirectory(${FW_1_SRC_DIRECTORY}) # QTPy
add_subdirectory(${FW_2_SRC_DIRECTORY}) # Pico
add_subdirectory(${FW_3_SRC_DIRECTORY}) # ProMicro
add_subdirectory(${FW_4_SRC_DIRECTORY}) # Tiny
add_subdirectory(${FW_5_SRC_DIRECTORY}) # Trinkey
