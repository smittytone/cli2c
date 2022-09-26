/**
 *
 * I2C driver w. HT16K33
 * Version 1.0.0
 * Copyright © 2022, smittytone
 * Licence: MIT
 *
 */
#include "main.h"


int main(int argc, char* argv[]) {
    
    // Process arguments
    if (argc < 2) {
        // Insufficient arguments -- issue usage info and bail
        printf("Usage: matrix {DEVICE_PATH} [I2C Address] [command] ... [command]\n");
    } else {
        // Instantiate an I2C data structure
        I2CDriver i2c;
        int i2c_address = HT16K33_I2C_ADDR;
        
        // Connect... with the device patj
        i2c_connect(&i2c, argv[1]);
        if (i2c.connected) {
            // Connected -- process the remaining commands in sequence
            int delta = 2;
            if (argc > 2) {
                char* token = argv[2];
                if (token[0] != '-') {
                    // Not a command, so an address
                    i2c_address = (int)strtol(token, NULL, 0);
                    
                    if (i2c_address < 0 || i2c_address > 0x7F) {
                        print_error("I2C address out of range");
                        return 1;
                    }
                    
                    // Note the non-standard I2C address
                    fprintf(stdout, "I2C address: 0x%02X\n", i2c_address);
                    delta = 3;
                }
                
                // Set up the display driver
                HT16K33_init(&i2c, i2c_address, HT16K33_0_DEG);
                
                // Process the commands one by one
                return matrix_commands(&i2c, argc, argv, delta);
            }
        } else {
            print_error("Could not connect to device %s\n", argv[1]);
        }
    }
    
    return 1;
}


int matrix_commands(I2CDriver* i2c, int argc, char* argv[], int delta) {
    
    for (int i = delta ; i < argc ; ++i) {
        char* command = argv[i];

        if (command[0] == '-') {
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
                                    return 1;
                                }

                                // Apply the command
                                HT16K33_set_brightness(brightness);
                                break;
                            }
                        }
                        
                        print_error("No brightness value supplied");
                        return 1;
                    }
                
                case 'c':   // DISPLAY CHARACTER
                    {
                        // Get the required argument
                        if (i < argc - 1) {
                            command = argv[++i];
                            if (command[0] != '-') {
                                long ascii = strtol(command, NULL, 0);

                                if (ascii < 32 || ascii > 127) {
                                    print_error("Ascii value out of range (32-127)");
                                    return 1;
                                }

                                // Get an optional argument
                                bool do_centre = false;
                                if (i < argc - 1) {
                                    command = argv[++i];
                                    if (command[0] != '-') {
                                        if (strlen(command) == 1) {
                                            do_centre = (strcmp(command, "1") == 0);
                                        } else {
                                            do_centre = (strcmp(command, "true") == 0);
                                        }
                                    } else {
                                        i -= 1;
                                    }
                                }

                                // Perform the action
                                HT16K33_set_char(ascii, do_centre);
                                HT16K33_draw();
                                break;
                            }
                        }

                        print_error("No Ascii value supplied");
                        return 1;
                    }

            case 'g':   // DISPLAY GLYPH
                {
                    // Get the required argument
                    if (i < argc - 1) {
                        command = argv[++i];
                        if (command[0] != '-') {
                            uint8_t bytes[8] = {0};
                            char *endptr = command;
                            size_t length = 0;

                            while (length < sizeof(bytes)) {
                                bytes[length++] = (uint8_t)strtol(endptr, &endptr, 0);
                                if (*endptr == '\0') break;
                                if (*endptr != ',') {
                                    print_error("Invalid bytes");
                                    return 1;
                                }

                                endptr++;
                            }
                            
                            // Perform the action
                            HT16K33_set_glyph(bytes);
                            HT16K33_draw();
                            break;
                        }
                    }

                    print_error("No Ascii value supplied");
                    return 1;
                }
            
            case 'h':   // HELP
                    show_help();
                    return 0;
            
            case 'p':   // PLOT A POINT
                    {
                        // Get two required arguments
                        long x = -1, y = -1;
                        if (i < argc - 1) {
                            command = argv[++i];
                            if (command[0] != '-') {
                                x = strtol(command, NULL, 0);
                                if (i < argc - 1) {
                                    command = argv[++i];
                                    if (command[0] != '-') {
                                        y = strtol(command, NULL, 0);

                                        if (x < 0 || x > 7 || y < 0 || y > 7) {
                                            print_error("Co-ordinate out of range (0-7)");
                                            return 1;
                                        }

                                        // Get an optional argument
                                        long ink = 1;
                                        if (i < argc - 1) {
                                            command = argv[++i];
                                            if (command[0] != '-') {
                                                ink = strtol(command, NULL, 0);
                                                if (ink != 1 && ink != 0) ink = 1;
                                            } else {
                                                i -= 1;
                                            }
                                        }

                                        // Perform the action
                                        HT16K33_plot((uint8_t)x, (uint8_t)y, ink == 1);
                                        HT16K33_draw();
                                        break;
                                    }
                                }
                            }
                        }

                        print_error("No co-ordinate value(s) supplied");
                        return 1;
                    }
                    break;

                case 't':     // TEXT STRING
                    {
                        // Get one required argument
                        if (i < argc - 1) {
                            char *scroll_string = argv[++i];

                            // Get an optional argument
                            long scroll_delay = 100;
                            if (i < argc - 1) {
                                command = argv[++i];
                                if (command[0] != '-') {
                                    scroll_delay = strtol(command, NULL, 0);
                                } else {
                                    i -= 1;
                                }
                            }
                            
                            // Perform the action
                            HT16K33_print(scroll_string, (uint32_t)scroll_delay);
                            break;
                        }

                        print_error("No string supplied");
                        return 1;
                    }
                    
                case 'z':   // TEST
                {
                    i2c_test(i2c);
                    break;
                }


                default:
                    // ERROR
                    print_error("Unknown command");
                    return 1;
            }
        }
    }
    
    return 0;
}


/**
 * @brief Show help.
 */
void show_help() {
    fprintf(stdout, "matrix {device} [address] [commands]\n\n");
    fprintf(stdout, "Commands:\n");
    fprintf(stdout, "  -a [on|off]             Activate/deactivate the display. Default: on.\n");
    fprintf(stdout, "  -b {0-15}               Set the display brightness from low (0) to high (15).\n");
    fprintf(stdout, "  -c {ascii} [true|false] Draw the Ascii character on the screen, and optionally\n");
    fprintf(stdout, "                          set it to be centred (true).\n");
    fprintf(stdout, "  -g {glyph}              Draw the user-defined character on the screen. The definition\n");
    fprintf(stdout, "                          is a string of eight comma-separated 8-bit hex values, eg.\n");
    fprintf(stdout, "                          '0x3C,0x42,0xA9,0x85,0x85,0xA9,0x42,0x3C'.\n");
    fprintf(stdout, "  -p {x} {y} [1|0]        Set or clear the specified pixel. X and Y coordinates are in\n");
    fprintf(stdout, "                          the range 0-7.\n");
    fprintf(stdout, "  -t {string} [delay]     Scroll the specified string. The second argument is an optional\n");
    fprintf(stdout, "                          delay be between column shifts in milliseconds. Default: 250ms.\n\n");
}


/**
 * @brief Issue an error message.
 *
 * @param format_string Message string with optional formatting
 * @param ...           Optional injectable values
 */
void print_error(char* format_string, ...) {
    va_list args;
    va_start(args, format_string);
    print_output(false, format_string, args);
    va_end(args);
}


/**
 * @brief Issue any message.
 *
 * @param is_err        Is the message an error?
 * @param format_string Message string with optional formatting
 * @param args          va_list of args from previous call
 */
void print_output(bool is_err, char* format_string, va_list args) {
    
    // Write the message type to the message
    char buffer[1024] = {0};
    sprintf(buffer, is_err ? "[ERROR] " : "[DEBUG] ");
    
    // Write the formatted text to the message
    vsnprintf(&buffer[8], sizeof(buffer) - 9, format_string, args);
    
    // Add NEWLINE to the message and output to UART
    sprintf(&buffer[strlen(buffer)], "\n");
    
    // Print it all out
    printf("%s", buffer);
}
