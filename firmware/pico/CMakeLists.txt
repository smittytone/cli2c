cmake_minimum_required(VERSION 3.18)

# Make project data accessible to compiler
# IMPORTANT `HW_MODEL` must be no more than 24 characters
add_compile_definitions(
    FW_NAME="${FW_2_NAME}"
    FW_VERSION="${FW_VERSION_NUMBER}"
    BUILD_NUM=${FW_BUILD_NUMBER}
    LED_BUILD=1
    HW_MODEL="RP-PI-PICO")

# Set the device's I2C, SPI pins
# These are for the board pins 4 (GPIO 2) and 5 (GPIO 3)
# You can set these to other I2C pins but make sure
# you select pins on I2C1
add_compile_definitions(
    DEFAULT_SDA_PIN=2
    DEFAULT_SCL_PIN=3
    DEFAULT_I2C_BUS=1
    DEFAULT_SPI_BUS=0
    DEFAULT_SPI_RX_PIN=0
    DEFAULT_SPI_TX_PIN=3
    DEFAULT_SPI_CS_PIN=1
    DEFAULT_SPI_SCK_PIN=2)

# Include app source code file(s)
add_executable(${FW_2_NAME}
    ${FW_2_SRC_DIRECTORY}/main.c
    ${FW_2_SRC_DIRECTORY}/pico_led.c
    ${FW_2_SRC_DIRECTORY}/pins.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
    ${COMMON_CODE_DIRECTORY}/i2c.c
    ${COMMON_CODE_DIRECTORY}/spi.c
    ${COMMON_CODE_DIRECTORY}/gpio.c)

# Compile debug segment LED sources
if (DO_DEBUG)
    target_sources(${FW_2_NAME} PRIVATE segment.c)
endif()

# Set the stack size
target_compile_definitions(${FW_2_NAME} PRIVATE PICO_STACK_SIZE=4096)

# Link to built libraries
target_link_libraries(${FW_2_NAME} LINK_PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_spi)

# Enable/disable STDIO via USB and UART
pico_enable_stdio_usb(${FW_2_NAME} 1)
pico_enable_stdio_uart(${FW_2_NAME} 0)

# Enable extra build products
pico_add_extra_outputs(${FW_2_NAME})
