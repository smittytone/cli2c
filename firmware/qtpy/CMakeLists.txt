cmake_minimum_required(VERSION 3.20)

# Make project data accessible to compiler
# IMPORTANT `HW_MODEL` must be no more than 16 characters
add_compile_definitions(
    FW_NAME="${FW_1_NAME}"
    FW_VERSION="${FW_1_VERSION_NUMBER}"
    BUILD_NUM=${BUILD_NUMBER}
    NEO_BUILD=1
    HW_MODEL="ADA-QTPY-RP2040")

# Set the device's I2C pins
# These are for the board pins A2 (SCL) and A3 (SDA)
# NOT SDA and SCL. This is to ensure comptatibility with
# the STEMMA port -- both A2,A3,STEMMA all used i2c1.

# Board pins A2 and A3
add_compile_definitions(SDA_PIN=26 SCL_PIN=27)
# STEMMA port
#add_compile_definitions(SDA_PIN=22 SCL_PIN=23)

add_compile_definitions(
    PIN_NEO_DATA=12
    PIN_NEO_PWR=11)

set(PICO_BOARD=adafruit_qtpy_rp2040)
set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=2000)

# Include app source code file(s)
add_executable(${FW_1_NAME}
    ${FW_1_SRC_DIRECTORY}/main.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
    ${COMMON_CODE_DIRECTORY}/segment.c
)

# Link to built libraries
target_link_libraries(${FW_1_NAME} LINK_PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_pio)

target_sources(${FW_1_NAME} PRIVATE ws2812.c)
pico_generate_pio_header(${FW_1_NAME} ${FW_1_SRC_DIRECTORY}/ws2812.pio)
add_compile_definitions(RGB_COLOUR=0x001000) # GREEN

# Enable/disable STDIO via USB and UART
pico_enable_stdio_usb(${FW_1_NAME} 1)
pico_enable_stdio_uart(${FW_1_NAME} 0)

# Enable extra build products
pico_add_extra_outputs(${FW_1_NAME})
