/*
 * I2C driver for HT16K33
 *
 * Version 0.1.2
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
                return 0;
            }
        }
        
        // Connect... with the device path
        i2c.port = -1;
        int i2c_address = HT16K33_I2C_ADDR;
        i2c_connect(&i2c, argv[1]);
        
        if (i2c.connected) {
            // Initialize the I2C host's I2C bus
            if (!(i2c_init(&i2c))) {
                print_error("%s could not initialise I2C\n", argv[1]);
                flush_and_close_port(i2c.port);
                return 1;
            }
            
            // Process the remaining commands in sequence
            int delta = 2;
            if (argc > 2) {
                char* token = argv[2];
                if (token[0] != '-') {
                    // Not a command, so an address
                    i2c_address = (int)strtol(token, NULL, 0);
                    
                    if (i2c_address < 0 || i2c_address > 0x7F) {
                        print_error("I2C address out of range");
                        flush_and_close_port(i2c.port);
                        return 1;
                    }
                    
                    // Note the non-standard I2C address
                    printf("I2C address: 0x%02X\n", i2c_address);
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
            print_error("Could not connect to device %s\n", argv[1]);
        }
    }
    
    return 1;
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
    
    for (int i = delta ; i < argc ; ++i) {
        char* command = argv[i];

        if (command[0] == '-') {
            switch (command[1]) {
                case 'a':   // ACTIVATE (DEFAULT) OR DEACTIVATE DISPLAY
                    {
                        // Check for and get the required argument
                        bool is_on = true;
                        if (i < argc - 1) {
                            command = argv[++i];
                            if (command[0] != '-') {
                                if (strlen(command) == 1) {
                                    is_on = (strcmp(command, "0") != 0);
                                } else {
                                    is_on = (strcmp(command, "off") != 0);
                                }
                                
                                // Get an optional argument
                                bool do_flip = false;
                                if (i < argc - 1) {
                                    command = argv[++i];
                                    if (command[0] != '-') {
                                        if (strlen(command) == 1) {
                                            do_flip = (strcmp(command, "1") == 0);
                                        } else {
                                            do_flip = (strcmp(command, "true") == 0);
                                        }
                                    } else {
                                        i -= 1;
                                    }
                                }
                                
                                // Apply the command
                                HT16K33_power(is_on);
                                if (do_flip) HT16K33_flip();
                                break;
                            }
                        }
                        
                        print_error("No power state value supplied (on/off)");
                        return 1;
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
                
                case 'g':   // DISPLAY GLYPH
                    {
                        // Get the required argument
                        if (i < argc - 1) {
                            command = argv[++i];
                            if (command[0] != '-') {
                                uint8_t glyph = (uint8_t)strtol(command, NULL, 0);

                                if (glyph > 0xFF) {
                                    print_error("Glyph value out of range (0-255)");
                                    return 1;
                                }
                                
                                // Get a required argument
                                if (i < argc - 1) {
                                    command = argv[++i];
                                    if (command[0] != '-') {
                                        uint8_t  digit =  (uint8_t)strtol(command, NULL, 0);

                                        if (digit > 3) {
                                            print_error("Digit value out of range (0-3)");
                                            return 1;
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
                                        HT16K33_draw();
                                        break;
                                    }
                                }
                                
                                print_error("No digit value supplied");
                                return 1;
                            }
                        }

                        print_error("No glyph value supplied");
                        return 1;
                    }
                
                case 'n':   // DISPLAY NUMBER
                    {
                        // Get the required argument
                        if (i < argc - 1) {
                            command = argv[++i];
                            if (command[0] != '-') {
                                long number = strtol(command, NULL, 0);

                                if (number < -999 || number > 9999) {
                                    print_error("Decimal value out of range (-999 to 9999)");
                                    return 1;
                                }

                                // Perform the action
                                HT16K33_show_value(number, false);
                                HT16K33_draw();
                                break;
                            }
                        }

                        print_error("No numbersupplied");
                        return 1;
                    }

                case 'w':
                    HT16K33_clear_buffer();
                    HT16K33_draw();
                    break;
                
                case '!':
                    i2c_commands(i2c, argc, argv, i);
                    break;
                    
                default:
                    // ERROR
                    print_error("Unknown command");
                    return 1;
            }
        } else {
            // Bad command
            print_error("Unknown command");
            return 1;
        }
    }
    
    return 0;
}


/**
 * @brief Show help.
 */
void show_help() {
    printf("segment {device} [address] [commands]\n\n");
    printf("Usage:\n");
    printf("  {device} is a mandatory device path, eg. /dev/cu.usbmodem-010101.\n");
    printf("  [address] is an optional display I2C address. Default: 0x70.\n");
    printf("  [commands] are optional HT16K33 matrix commands, as shown below.\n\n");
    printf("Commands:\n");
    printf("  -a [on|off]  Activate/deactivate the display. Default: on.\n");
    printf("  -b {0-15}    Set the display brightness from low (0) to high (15).\n");
    printf("  -n {number}  Draw the decimal number on the screen. Range -999 to 9999.\n");
    printf("  -g {glyph}   Draw the user-defined character on the screen. The definition\n");
    printf("               is a byte with bits set for each of the digit’s segments.\n");
    printf("  -w           Wipe (clear) the display.\n\n");
}


/**
 * @brief Issue an error message.
 *
 * @param format_string: Message string with optional formatting
 * @param ...:           Optional injectable values
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
 * @param is_err:        Is the message an error?
 * @param format_string: Message string with optional formatting
 * @param args:          va_list of args from previous call
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


/**
 * @brief Callback for Ctrl-C.
 */
void ctrl_c_handler(int dummy) {
    
    if (i2c.port != -1) flush_and_close_port(i2c.port);
    printf("\n");
    exit(0);
}
