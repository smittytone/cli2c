cmake_minimum_required(VERSION 3.20)

# Uncomment the following line to use the STEMMA
#set(USE_STEMMA=1)

# Make project data accessible to compiler
# IMPORTANT `HW_MODEL` must be no more than 16 characters
add_compile_definitions(
    FW_NAME="${FW_3_NAME}"
    FW_VERSION="${FW_3_VERSION_NUMBER}"
    BUILD_NUM=${BUILD_NUMBER}
    NEO_BUILD=1
    HW_MODEL="SF-PRO-MICRO")

# Set the device's I2C pins
# These are for the board pins 5 (GPIO 2) and 6 (GPIO 3)
# You can set these to other I2C pins but make sure
# you select pins on I2C1
if (DEFINED USE_STEMMA)
    add_compile_definitions(
        SDA_PIN=16
        SCL_PIN=17)
else()
    add_compile_definitions(
        SDA_PIN=2
        SCL_PIN=3)
endif()

# NeoPixel
add_compile_definitions(
    PIN_NEO_DATA=25
    PIN_NEO_PWR=7
    NEO_COLOUR=0x000010)

# Include app source code file(s)
add_executable(${FW_3_NAME}
    ${FW_3_SRC_DIRECTORY}/main.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
    ${COMMON_CODE_DIRECTORY}/segment.c
)

# Set the stack size
target_compile_definitions(${FW_3_NAME} PRIVATE PICO_STACK_SIZE=4096)

# Link to built libraries
target_link_libraries(${FW_3_NAME} LINK_PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_pio)

target_sources(${FW_3_NAME} PRIVATE ${FW_1_SRC_DIRECTORY}/ws2812.c)
pico_generate_pio_header(${FW_3_NAME} ${FW_1_SRC_DIRECTORY}/ws2812.pio)
#add_compile_definitions(RGB_COLOUR=0x001000) # GREEN

# Enable/disable STDIO via USB and UART
pico_enable_stdio_usb(${FW_3_NAME} 1)
pico_enable_stdio_uart(${FW_3_NAME} 0)

# Enable extra build products
pico_add_extra_outputs(${FW_3_NAME})
