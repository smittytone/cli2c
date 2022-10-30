/*
 * Bus Host Firmware -- SparkFun ProMicro
 *
 * @version     2.0.0
 * @author      Tony Smith (@smittytone)
 * @copyright   2022
 * @licence     MIT
 *
 */
#include <stdint.h>

uint8_t I2C_PIN_PAIRS_BUS_0[] = {   4, 5,
                                    8, 9,
                                    16, 17,
                                    20, 21,
                                    255, 255};

uint8_t I2C_PIN_PAIRS_BUS_1[] = {   2, 3,
                                    6, 7,
                                    26, 27,
                                    255, 255};

uint8_t SPI_PIN_QUADS_BUS_0[] = {   0, 3, 1, 2,
                                    4, 7, 5, 6};

uint8_t SPI_PIN_QUADS_BUS_1[] = {   20, 23, 21, 22};