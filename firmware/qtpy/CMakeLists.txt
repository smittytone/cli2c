cmake_minimum_required(VERSION 3.18)

# Uncomment the following line to use the STEMMA
set(USE_STEMMA 1)

# Make project data accessible to compiler
# IMPORTANT `HW_MODEL` must be no more than 24 characters
add_compile_definitions(
    FW_NAME="${FW_1_NAME}"
    FW_VERSION="${FW_VERSION_NUMBER}"
    BUILD_NUM=${FW_BUILD_NUMBER}
    NEO_BUILD=1
    HW_MODEL="ADA-QTPY-RP2040")

# I2C
# These are for the board pins A2 (SCL) and A3 (SDA)
# NOT SDA and SCL. This is to ensure comptatibility with
# the STEMMA port -- both A2,A3,STEMMA all used i2c1.
if (USE_STEMMA)
    add_compile_definitions(
        DEFAULT_SDA_PIN=22
        DEFAULT_SCL_PIN=23
        DEFAULT_I2C_BUS=1)
    unset(USE_STEMMA)
else()
    add_compile_definitions(
        DEFAULT_SDA_PIN=28
        DEFAULT_SCL_PIN=29
        DEFAULT_I2C_BUS=0)
endif()

# SPI
add_compile_definitions(
    DEFAULT_SPI_BUS=1
    DEFAULT_SPI_RX_PIN=4
    DEFAULT_SPI_TX_PIN=7
    DEFAULT_SPI_CS_PIN=5
    DEFAULT_SPI_SCK_PIN=6)

# NeoPixel
add_compile_definitions(
    PIN_NEO_DATA=12
    PIN_NEO_PWR=11
    NEO_COLOUR=0x100010)    # Magenta

set(PICO_BOARD=adafruit_qtpy_rp2040)
set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=2000)

# Include app source code file(s)
add_executable(${FW_1_NAME}
    ${FW_1_SRC_DIRECTORY}/main.c
    ${FW_1_SRC_DIRECTORY}/pins.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
    ${COMMON_CODE_DIRECTORY}/i2c.c
    ${COMMON_CODE_DIRECTORY}/spi.c
    ${COMMON_CODE_DIRECTORY}/gpio.c)

# Compile debug segment LED sources
if (DO_DEBUG)
    target_sources(${FW_1_NAME} PRIVATE segment.c)
endif()

# Link to built libraries
target_link_libraries(${FW_1_NAME} LINK_PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_pio)

# Compile WS2828 sources
target_sources(${FW_1_NAME} PRIVATE ws2812.c)
pico_generate_pio_header(${FW_1_NAME} ${FW_1_SRC_DIRECTORY}/ws2812.pio)

# Enable/disable STDIO via USB and UART
pico_enable_stdio_usb(${FW_1_NAME} 1)
pico_enable_stdio_uart(${FW_1_NAME} 0)

# Enable extra build products
pico_add_extra_outputs(${FW_1_NAME})
