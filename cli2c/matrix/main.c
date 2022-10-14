/*
 * I2C driver for an HT16K33 8x8 Matrix Display
 *
 * Version 0.1.6
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
        printf("Usage: matrix {DEVICE_PATH} [I2C Address] [command] ... [command]\n");
    } else {
        // Check for a help request
        for (int i = 0 ; i < argc ; ++i) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
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
                HT16K33_init(&i2c, i2c_address, HT16K33_0_DEG);
                
                // Process the commands one by one
                int result =  matrix_commands(&i2c, argc, argv, delta);
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
int matrix_commands(I2CDriver* i2c, int argc, char* argv[], int delta) {
    
    bool do_draw = false;
    
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
                
                case 'c':   // DISPLAY CHARACTER
                    {
                        // Get the required argument
                        if (i < argc - 1) {
                            command = argv[++i];
                            if (command[0] != '-') {
                                long ascii = strtol(command, NULL, 0);

                                if (ascii < 32 || ascii > 127) {
                                    print_error("Ascii value out of range (32-127)");
                                    return EXIT_ERR;
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
                                do_draw = true;
                                break;
                            }
                        }

                        print_error("No Ascii value supplied");
                        return EXIT_ERR;
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
                                        return EXIT_ERR;
                                    }

                                    endptr++;
                                }
                                
                                // Perform the action
                                HT16K33_set_glyph(bytes);
                                do_draw = true;
                                break;
                            }
                        }

                        print_error("No glyph value supplied");
                        return EXIT_ERR;
                    }
            
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
                                            return EXIT_ERR;
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
                                        do_draw = true;
                                        break;
                                    }
                                }
                            }
                        }

                        print_error("No co-ordinate value(s) supplied");
                        return EXIT_ERR;
                    }
                    break;

                case 'r':       // ROTATE DISPLAY
                    {
                        if (i < argc - 1) {
                            long angle = 0;
                            command = argv[++i];
                            
                            if (command[0] != '-') {
                                angle = strtol(command, NULL, 0);
                            } else {
                                i -= 1;
                            }
                            
                            // Perform the action
                            HT16K33_set_angle((uint8_t)angle);
                            HT16K33_rotate((uint8_t)angle);
                        }
                    }
                    break;
                
                case 't':     // SCROLL A TEXT STRING
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
        } else {
            // Bad command
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
    printf("matrix {device} [address] [commands]\n\n");
    printf("Usage:\n");
    printf("  {device} is a mandatory device path, eg. /dev/cu.usbmodem-010101.\n");
    printf("  [address] is an optional display I2C address. Default: 0x70.\n");
    printf("  [commands] are optional HT16K33 matrix commands:\n\n");
    printf("Commands:\n");
    printf("  -a [on|off]             Activate/deactivate the display. Default: on.\n");
    printf("  -b {0-15}               Set the display brightness from low (0) to high (15).\n");
    printf("  -r {0-3}                Rotate the display. Angle supplied as a multiple of 90 degrees.\n");
    printf("  -c {ascii} [true|false] Draw the Ascii character on the screen, and optionally\n");
    printf("                          set it to be centred (true).\n");
    printf("  -g {glyph}              Draw the user-defined character on the screen. The definition\n");
    printf("                          is a string of eight comma-separated 8-bit hex values, eg.\n");
    printf("                          '0x3C,0x42,0xA9,0x85,0x85,0xA9,0x42,0x3C'.\n");
    printf("  -p {x} {y} [1|0]        Set or clear the specified pixel. X and Y coordinates are in\n");
    printf("                          the range 0-7.\n");
    printf("  -t {string} [delay]     Scroll the specified string. The second argument is an optional\n");
    printf("                          delay be between column shifts in milliseconds. Default: 250ms.\n");
    printf("  -w                      Wipe (clear) the display.\n\n");
}
