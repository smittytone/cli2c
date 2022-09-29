cmake_minimum_required(VERSION 3.20)

# Make project data accessible to compiler
add_compile_definitions(FW_NAME="${FW_2_NAME}")
add_compile_definitions(FW_VERSION="${FW_2_VERSION_NUMBER}")
add_compile_definitions(BUILD_NUM=${BUILD_NUMBER})
add_compile_definitions(PICO_BUILD=1)
add_compile_definitions(HW_MODEL="PI-PICO")

# Include app source code file(s)
add_executable(${FW_2_NAME}
    ${FW_2_SRC_DIRECTORY}/main.c
    ${FW_2_SRC_DIRECTORY}/pico_led.c
    ${COMMON_CODE_DIRECTORY}/serial.c
    ${COMMON_CODE_DIRECTORY}/led.c
)

# Link to built libraries
target_link_libraries(${FW_2_NAME} LINK_PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_pio)

# Enable/disable STDIO via USB and UART
pico_enable_stdio_usb(${FW_2_NAME} 1)
pico_enable_stdio_uart(${FW_2_NAME} 0)

# Enable extra build products
pico_add_extra_outputs(${FW_2_NAME})
