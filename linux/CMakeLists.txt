cmake_minimum_required(VERSION 3.18)

# Set project data
set(PROJECT_NAME "cli2c for Linux")
set(VERSION_NUMBER "1.1.2")
set(BUILD_NUMBER "34")

# Set app name(s) and version(s)
set(CLI_CODE_DIRECTORY "${CMAKE_SOURCE_DIR}/../cli2c/cli2c")
set(MATRIX_CODE_DIRECTORY "${CMAKE_SOURCE_DIR}/../cli2c/matrix")
set(SEGMENT_CODE_DIRECTORY "${CMAKE_SOURCE_DIR}/../cli2c/segment")
set(COMMON_CODE_DIRECTORY "${CMAKE_SOURCE_DIR}/../cli2c/common")

# Set flags and directory variables
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DTSDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTSDEBUG")

add_compile_definitions(BUILD_FOR_LINUX=1)
#add_compile_definitions(DEBUG=1)

include_directories(
    ${COMMON_CODE_DIRECTORY}
    ${CLI_CODE_DIRECTORY}
    ${MATRIX_CODE_DIRECTORY} 
    ${SEGMENT_CODE_DIRECTORY})

# Name the project
project(${PROJECT_NAME}
        LANGUAGES C
        VERSION ${VERSION_NUMBER}
        DESCRIPTION "Command line I2C access"
        HOMEPAGE_URL https://smittytone.net/cli2c)

# Set the build version
add_compile_definitions(APP_VERSION="${VERSION_NUMBER}")

# Include app source code file(s)
add_executable(cli2c
    ${CLI_CODE_DIRECTORY}/main.c
    ${COMMON_CODE_DIRECTORY}/i2cdriver.c
    ${COMMON_CODE_DIRECTORY}/utils.c)

add_executable(matrix
    ${MATRIX_CODE_DIRECTORY}/main.c
    ${MATRIX_CODE_DIRECTORY}/ht16k33-matrix.c
    ${COMMON_CODE_DIRECTORY}/i2cdriver.c
    ${COMMON_CODE_DIRECTORY}/utils.c)

add_executable(segment
    ${SEGMENT_CODE_DIRECTORY}/main.c
    ${SEGMENT_CODE_DIRECTORY}/ht16k33-segment.c
    ${COMMON_CODE_DIRECTORY}/i2cdriver.c
    ${COMMON_CODE_DIRECTORY}/utils.c)