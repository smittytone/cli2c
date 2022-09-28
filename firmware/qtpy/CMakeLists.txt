cmake_minimum_required(VERSION 3.20)

# Make project data accessible to compiler
add_compile_definitions(FW_NAME="${FW_1_NAME}")
add_compile_definitions(FW_VERSION="${FW_1_VERSION_NUMBER}")
add_compile_definitions(BUILD_NUM=${BUILD_NUMBER})
add_compile_definitions(QTPY_BUILD=1)
add_compile_definitions(HW_MODEL="QTPY-RP2040")

# Include app source code file(s)
add_executable(${FW_1_NAME}
    ${FW_1_SRC_DIRECTORY}/main.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
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
