cmake_minimum_required(VERSION 3.18)

# Make project data accessible to compiler
# IMPORTANT `HW_MODEL` must be no more than 24 characters
add_compile_definitions(
    FW_NAME="${FW_4_NAME}"
    FW_VERSION="${FW_VERSION_NUMBER}"
    BUILD_NUM=${FW_BUILD_NUMBER}
    TINY_BUILD=1
    HW_MODEL="PIMORONI-TINY")

# Set the device's I2C pins
add_compile_definitions(
    DEFAULT_SDA_PIN=6
    DEFAULT_SCL_PIN=7
    DEFAULT_I2C_BUS=1)

set(PICO_BOARD=pimoroni_tiny_2040)
set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=2000)

# Include app source code file(s)
add_executable(${FW_4_NAME}
    ${FW_4_SRC_DIRECTORY}/main.c
    ${FW_4_SRC_DIRECTORY}/tiny_led.c
    ${FW_4_SRC_DIRECTORY}/pins.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
    ${COMMON_CODE_DIRECTORY}/gpio.c
    ${COMMON_CODE_DIRECTORY}/i2c.c)

# Compile debug sources
if (DO_DEBUG)
    target_sources(${FW_4_NAME} PRIVATE ${COMMON_CODE_DIRECTORY}/debug.c)
endif()

# Set the stack size
target_compile_definitions(${FW_4_NAME} PRIVATE
    PICO_HEAP_SIZE=8192
    PICO_STACK_SIZE=8192)

# Link to built libraries
target_link_libraries(${FW_4_NAME} LINK_PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_pwm)

# Enable/disable STDIO via USB and UART
pico_enable_stdio_usb(${FW_4_NAME} 1)
pico_enable_stdio_uart(${FW_4_NAME} 0)

# Enable extra build products
pico_add_extra_outputs(${FW_4_NAME})
