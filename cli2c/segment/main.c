/*
 * I2C driver for an HT16K33 4-digit, 7-segment display
 *
 * Version 1.0.0
 * Copyright © 2022, Tony Smith (@smittytone)
 * Licence: MIT
 *
 */
#include "main.h"


// Hold an I2C data structure
I2CDriver i2c;


int main(int argc, char* argv[]) {

    // Listen for SIGINT
    signal(SIGINT, ctrl_c_handler);

    // Process arguments
    if (argc < 2) {
        // Insufficient arguments -- issue usage info and bail
        fprintf(stderr, "Usage: matrix {DEVICE_PATH} [I2C Address] [command] ... [command]\n");
    } else {
        // Check for a help request
        for (int i = 0 ; i < argc ; ++i) {
            lower(argv[i]);
            if (strcmp(argv[i], "h") == 0 ||
                strcmp(argv[i], "-h") == 0 ||
                strcmp(argv[i], "--help") == 0) {
                show_help();
                return EXIT_OK;
            }
        }

        // Connect... with the device path
        i2c.port = -1;
        int i2c_address = HT16K33_I2C_ADDR;
        i2c_connect(&i2c, argv[1]);

        if (i2c.connected) {
            // Initialize the I2C host's I2C bus
            if (!(i2c_init(&i2c))) {
                print_error("%s could not initialise I2C", argv[1]);
                flush_and_close_port(i2c.port);
                return EXIT_ERR;
            }

            // Process the remaining commands in sequence
            int delta = 2;
            if (argc > 2) {
                char* token = argv[2];
                if (token[0] != '-') {
                    // Not a command, so an address?
                    i2c_address = (int)strtol(token, NULL, 0);

                    // Only allow legal I2C address range
                    if (i2c_address < 0x08 || i2c_address > 0x77) {
                        print_error("I2C address out of range");
                        flush_and_close_port(i2c.port);
                        return EXIT_ERR;
                    }

                    // Note the non-standard I2C address
                    print_warning("Using I2C address: 0x%02X", i2c_address);
                    delta = 3;
                }

                // Set up the display driver
                HT16K33_init(&i2c, i2c_address);

                // Process the commands one by one
                int result =  segment_commands(&i2c, argc, argv, delta);
                flush_and_close_port(i2c.port);
                return result;
            }
        } else {
            print_error("Could not connect to device %s", argv[1]);
        }
    }

    return EXIT_ERR;
}


/**
 * @brief Parse and process commands for the HT16K33-based matrix.
 *
 * @param i2c:   An I2C driver data structure.
 * @param argc:  The argument count.
 * @param argv:  The arguments.
 * @param delta: The argument list offset to locate HT16K33 commands from.
 */
int segment_commands(I2CDriver* i2c, int argc, char* argv[], int delta) {

    bool do_draw = false;

    for (int i = delta ; i < argc ; ++i) {
        char* command = argv[i];
        switch (command[1]) {
            case 'a':   // ACTIVATE (DEFAULT) OR DEACTIVATE DISPLAY
                {
                    // Check for and get the optional argument
                    bool is_on = true;
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            if (strlen(command) == 1) {
                                is_on = (strcmp(command, "0") != 0);
                            } else {
                                is_on = (strcmp(command, "off") != 0);
                            }
                        } else {
                            i -= 1;
                        }
                    }

                    // Apply the command
                    HT16K33_power(is_on);
                }
                break;

            case 'b':   // SET BRIGHTNESS
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            long brightness = strtol(command, NULL, 0);

                            if (brightness < 0 || brightness > 15) {
                                print_error("Brightness value out of range (0-15)");
                                return EXIT_ERR;
                            }

                            // Apply the command
                            HT16K33_set_brightness(brightness);
                            break;
                        }
                    }

                    print_error("No brightness value supplied");
                    return EXIT_ERR;
                }

            case 'c':   // DISPLAY A CHARACTER ON A DIGIT
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            char achar = command[0];

                            // Get a required argument
                            if (i < argc - 1) {
                                command = argv[++i];
                                if (command[0] != '-') {
                                    uint8_t digit = (uint8_t)strtol(command, NULL, 0);

                                    if (digit > 3) {
                                        print_error("Digit value out of range (0-3)");
                                        return EXIT_ERR;
                                    }

                                    // Get an optional argument
                                    bool show_point = false;
                                    if (i < argc - 1) {
                                        command = argv[++i];
                                        if (command[0] != '-') {
                                            if (strlen(command) == 1) {
                                                show_point = (strcmp(command, "1") == 0);
                                            } else {
                                                show_point = (strcmp(command, "true") == 0);
                                            }
                                        } else {
                                            i -= 1;
                                        }
                                    }

                                    // Perform the action
                                    HT16K33_set_char(achar, digit, false);
                                    do_draw = true;
                                    break;
                                }
                            }

                            print_error("No digit value supplied");
                            return EXIT_ERR;
                        }
                    }

                    print_error("No glyph value supplied");
                    return EXIT_ERR;
                }

            case 'd':   // SET DECIMAL POINT ON A DIGIT (0-3)
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            uint8_t digit = (uint8_t)strtol(command, NULL, 0);

                            if (digit > 3) {
                                print_error("Digit value out of range (0-3)");
                                return EXIT_ERR;
                            }

                            // Apply the command
                            HT16K33_set_point(digit);
                            do_draw = true;
                            break;
                        }
                    }

                    print_error("No digit value supplied");
                    return EXIT_ERR;
                }

            case 'f':   // FLIP DISPLAY
                HT16K33_flip();
                break;

            case 'g':   // DISPLAY A GLYPH ON A DIGIT
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            uint8_t glyph = (uint8_t)strtol(command, NULL, 0);

                            if (glyph > 0xFF) {
                                print_error("Glyph value out of range (0-255)");
                                return EXIT_ERR;
                            }

                            // Get a required argument
                            if (i < argc - 1) {
                                command = argv[++i];
                                if (command[0] != '-') {
                                    uint8_t digit = (uint8_t)strtol(command, NULL, 0);

                                    if (digit > 3) {
                                        print_error("Digit value out of range (0-3)");
                                        return EXIT_ERR;
                                    }

                                    // Get an optional argument
                                    bool show_point = false;
                                    if (i < argc - 1) {
                                        command = argv[++i];
                                        if (command[0] != '-') {
                                            if (strlen(command) == 1) {
                                                show_point = (strcmp(command, "1") == 0);
                                            } else {
                                                show_point = (strcmp(command, "true") == 0);
                                            }
                                        } else {
                                            i -= 1;
                                        }
                                    }

                                    // Perform the action
                                    HT16K33_set_glyph(glyph, digit, false);
                                    do_draw = true;
                                    break;
                                }
                            }

                            print_error("No digit value supplied");
                            return EXIT_ERR;
                        }
                    }

                    print_error("No glyph value supplied");
                    return EXIT_ERR;
                }

            case 'k':   // SET OR UNSET THE COLON
                HT16K33_set_colon();
                do_draw = true;
                break;

            case 'n':   // DISPLAY A NUMBER ACROSS THE DISPLAY
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-' || (command[0] == '-' && command[1] >= '0' && command[1] <= '9')) {
                            int number = (int)strtol(command, NULL, 0);

                            if (number < -999 || number > 9999) {
                                print_error("Decimal value out of range (-999 to 9999)");
                                return EXIT_ERR;
                            }

                            // Perform the action
                            HT16K33_show_value(number, false);
                            do_draw = true;
                            break;
                        }
                    }

                    print_error("No number supplied");
                    return EXIT_ERR;
                }

            case 'v':   // DISPLAY A VALUE ON A DIGIT
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            uint8_t number = (uint8_t)strtol(command, NULL, 0);

                            if (number > 0x0F) {
                                print_error("Value out of range (00-0F)");
                                return EXIT_ERR;
                            }

                            // Get a required argument
                            if (i < argc - 1) {
                                command = argv[++i];
                                if (command[0] != '-') {
                                    uint8_t  digit =  (uint8_t)strtol(command, NULL, 0);

                                    if (digit > 3) {
                                        print_error("Digit value out of range (0-3)");
                                        return EXIT_ERR;
                                    }

                                    // Get an optional argument
                                    bool show_point = false;
                                    if (i < argc - 1) {
                                        command = argv[++i];
                                        if (command[0] != '-') {
                                            if (strlen(command) == 1) {
                                                show_point = (strcmp(command, "1") == 0);
                                            } else {
                                                show_point = (strcmp(command, "true") == 0);
                                            }
                                        } else {
                                            i -= 1;
                                        }
                                    }

                                    // Perform the action
                                    HT16K33_set_number(number, digit, false);
                                    do_draw = true;
                                    break;
                                }
                            }

                            print_error("No digit value supplied");
                            return EXIT_ERR;
                        }
                    }

                    print_error("No glyph value supplied");
                    return EXIT_ERR;
                }

            case 'w':
                HT16K33_clear_buffer();
                do_draw = true;
                break;

            case 'z':
                do_draw = true;
                break;

            case '!':
                i2c_commands(i2c, argc, argv, i);
                break;

            default:
                // ERROR
                print_error("Unknown command");
                return EXIT_ERR;
        }
    }

    if (do_draw) HT16K33_draw();
    return EXIT_OK;
}


/**
 * @brief Show help.
 */
void show_help() {
    fprintf(stderr, "segment {device} [address] [commands]\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  {device} is a mandatory device path, eg. /dev/cu.usbmodem-010101.\n");
    fprintf(stderr, "  [address] is an optional display I2C address. Default: 0x70.\n");
    fprintf(stderr, "  [commands] are optional HT16K33 segment commands.\n\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  a [on|off]                      Activate/deactivate the display. Default: on.\n");
    fprintf(stderr, "  b {0-15}                        Set the display brightness from low (0) to high (15).\n");
    fprintf(stderr, "  f                               Flip the display vertically.\n");
    fprintf(stderr, "  n {number}                      Draw the decimal number on the screen.\n");
    fprintf(stderr, "                                  Range -999 to 9999.\n");
    fprintf(stderr, "  v {value} {digit} [true|false]  Draw the value on the screen at the specified digit\n");
    fprintf(stderr, "                                  (0-15/0x00-0x0F) and optionally set its decimal point.\n");
    fprintf(stderr, "  g {glyph} {digit} [true|false]  Draw the user-defined character on the screen at the\n");
    fprintf(stderr, "                                  specified digit. The glyph definition is a byte with bits\n");
    fprintf(stderr, "                                  set for each of the digit’s segments.\n");
    fprintf(stderr, "  w                               Wipe (clear) the display.\n");
    fprintf(stderr, "  h                               Help information.\n\n");
}
