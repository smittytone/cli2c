# cli2c 1.2.0

An I2C client for macOS and Linux. It is used as the basis for client apps to operate HT16K33-controlled matrix and segment LED displays.

It requires a [Raspberry Pi Pico](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html), [Adafruit QTPy RP2040](https://www.adafruit.com/product/4900), [Adafruit QT2040 Trinkey](https://www.adafruit.com/product/5056), [SparkFun ProMicro RP2040](https://www.sparkfun.com/products/18288) or [Pimoroni Tiny 2040](https://shop.pimoroni.com/products/tiny-2040?variant=39560012234835) connected via USB and running the included firmware. This is the I2C Host.

There’s more information [in this blog post](https://blog.smittytone.net/2022/10/18/how-to-talk-to-i2c-sensors-displays-from-a-mac/).

**IMPORTANT** Version 1.2.0 of the apps require version 1.2.0 of the host firmware; they will not work with older versions of the firmware. Firmware 1.2.0 will work with older versions of the apps, but these versions are deprecated and support for them will be removed in a future version of the firmware.

## Build the Client Apps

#### macOS

You can build the code from the accompanying Xcode project.

1. Archive the project.
1. Save the build products on the desktop.
1. Copy the binaries `cli2c`, `matrix` and `segment` to your preferred location listed in `$PATH`.

#### Linux

1. Navigate to the repo directory.
1. `cd linux`
1. `cmake -S . -B build`
1. `cmake --build build`
1. Copy the binaries `cli2c`, `matrix` and `segment` to your preferred location listed in `$PATH`.

## Build and Deploy the I2C Host Firmware

1. Navigate to the repo directory.
1. `cmake -S . -B firmwarebuild`
1. `cmake --build firmwarebuild`
1. Write the firmware depending on which board you are using:
    * `./deploy.sh /path/to/device firmwarebuild/firmware/pico/firmware_pico_rp2040.uf2`
    * `./deploy.sh /path/to/device firmwarebuild/firmware/qtpy/firmware_qtpy_rp2040.uf2`
    * `./deploy.sh /path/to/device firmwarebuild/firmware/promicro/firmware_promicro.uf2`
    * `./deploy.sh /path/to/device firmwarebuild/firmware/tiny/firmware_tiny2040.uf2`
    * `./deploy.sh /path/to/device firmwarebuild/firmware/trinkey/firmware_trinkey2040.uf2`

The deploy script tricks the RP2040-based board into booting into disk mode, then copies over the newly build firmware. When the copy completes, the RP2040 automatically reboots. This saves of a lot of tedious power-cycling with the BOOT button held down.

## What’s What

The contents of this repo are:

```
/cli2c
|
|___/cli2c                      // The macOS driver and related code
|   |___/cli2c                  // A generic CLI tool for any I2C device
|   |___/matrix                 // An HT16K33 8x8 matrix-oriented version of cli2c
|   |___/segment                // An HT16K33 4-digit, 7-segment-oriented version of cli2c
|   |___/common                 // Code common to all versions
|
|___/firmware                   // The RP2040 host firmware, written in C
|   |___/pico                   // The Raspberry Pi Pico version
|   |___/qtpy                   // An Adafruit QTPy RP2040 version
|   |___/promicro               // A SparkFun ProMicro RP2040 version
|   |___/tiny                   // A Pimoroni Tiny 2040 version
|   |___/trinkey                // An Adafruit QT2040 Trinkey version
|   |___/common                 // Code common to all versions
|
|___/examples                   // Demo apps
|   |___cpu_chart_matrix.py     // CPU utilization display for 8x8 matrix LEDs
|   |___cpu_chart_segment.py    // CPU utilization display for 4-digit segment LEDs
|   |___mcp9809_temp.py         // Periodic temperature reports from an MCP9808 sensor
|
|___/linux                      // Linux build settings (Cmake) for the driver and client apps
|
|___CMakeLists.txt              // Top-level firmware project CMake config file
|___pico_sdk_import.cmake       // Raspberry Pi Pico SDK CMake import script
|
|___firmware.code-workspace     // Visual Studio Code workspace for the RP2040 firmware
|___cli2c.xcodeproj             // Xcode workspace for cli2c, matrix and segment
|
|___deploy.sh                   // A .uf2 deployment script that saves pressing
|                               // RESET/BOOTSEL buttons.
|
|___README.md
|___LICENSE.md
```

## Devices

Under macOS, RP2040-based boards will appear in `/dev` as `cu.usbmodemXXXXX` or `cu.usbserialXXXXXX`. You can use my [`dlist()`](https://blog.smittytone.net/2022/09/08/how-to-manage-serial-devices-on-mac/) shell function to save looking up and keying in these names, which can vary across boots.

Under Linux, specifically Raspberry Pi OS, boards appear as `/dev/ttyACM0`. You may need to add your user account to the group `dialout` in order to access the port:

1. Run `sudo adduser smitty dialout`
1. Log out and then back in again.

## cli2c

`cli2c` is a command line driver for the USB-connected RP2040-based I2C host, which must be pre-loaded with the firmware.

It is a generic I2C driver with the following syntax:

```shell
cli2c {device_port} [command] ... [command]
```

**Note** Arguments in braces `{}` are required; those in square brackets `\[\]` are optional.

* `device_port` is the USB-connected I2C host’s Unix device path, eg. `/dev/cu.usbmodem-101`.
* [command] is an optional command block, comprising a single-character command and any required data as described in the following table.

| Command | Arguments | Description |
| :-: | :-: | --- |
| `z` |  | Initialise the target I2C bus. The bus is not initialised at startup |
| `c` | `{bus}` `{sda_pin}` `{scl_pin}` | Choose the host’s I2C bus (0 or 1) and the SDA and SCL pins by their RP2040 GPIO number. Defaults are board-specific and shown below. You must select a bus before initialising it |
| `f` | {frequency} | The I2C bus frequency in multiples of 100kHz. Supported values: 1 and 4 |
| `w` | `{address}` `{data_bytes}` | Write the supplied data to the I2C device at `address`. `data_bytes` are comma-separated 8-bit hex values, eg. `0x4A,0x5C,0xFF` |
| `r` | `{address}` `{count}` | Read `count` bytes from the I2C device at `address` and issue an I2C STOP |
| `p` |  | Issue an I2C STOP. Usually used after one or more writes |
| `x` |  | Reset the I2C bus |
| `s` |  |  Display devices on the I2C bus. **Note** This will initialise the bus if it is not already initialised |
| `i` |  |  Display I2C host device information |
| `g` | {pin_number} [hi|lo] [in|out] | [Set a GPIO pin](#gpio). |
| `l` | {`on`\|`off`} | Turn the I2C Host LED on or off |
| `h` |  |  Display help information |

#### Error and Data Output

All message output is routed via `stderr`. All data read back from an I2C device is output to `stdout` so it can be captured to a file. Returned data is currently presented as hexadecimal strings. For example:

```shell
cli2c /dev/cu.usbmodem-101 r 0x18 16 > data.bin
cat data.bin
A0FF123B5C5FAA5F010304
```

#### Device Defaults

| Board | I2C Bus | SDA Pin | SCL Pin |
| --- | :-: | :-: | :-: |
| Pico | 1 | 2 | 3 |
| QTpy&#42; | 1 | 22 (26) | 23 (27) |
| Trinkey | 0 | 16 | 17 |
| ProMicro&#42; | 1 (0) | 2 (16) | 3 (17) |
| Tiny | 1 | 6 | 7 |

Devices marked with &#42; have a STEMMA-QT port as well as pins. The values in brackets are the bus and pins used when the STEMMA-QT is set to be the default. This can be done uncommenting the

```
set(USE_STEMMA 1)
```

line in the board’s `CMakeLists.txt` file.

But don’t forget that you can use the `c` command to choose an alternative I2C bus and pins.

#### GPIO

You can use `cli2c` to set a GPIO pin. Provide the GPIO number and its initial state (`hi` or `lo`) and its mode: whether it is an input or output (`in` or `out`).

Setting a pin to an output immediately sets its state to the provided value. To change the state, just pass in the opposite value. There’s no need to specify its mode again unless you wish to change it.

```
cli2c /dev/cu.usbmodem-101 10 hi out
cli2c /dev/cu.usbmodem-101 10 lo
```

Setting a pin to an input does nothing immediately, and the supplied state value is ignored unless it is `r`, for read. To read an input pin, pass `r` in place of a state: `cli2c` will output the pin’s current value as a two-digit hex number:

```
cli2c /dev/cu.usbmodem-101 10 lo in
cli2c /dev/cu.usbmodem-101 10 r
01
```

Again, you don’t need to restate the pin’s mode unless you’re changing it.

## matrix

`matrix` is a specific driver for HT16K33-based 8x8 LED matrices. It embeds `cli2c` but exposes a different, display-oriented set of commands.

You use the driver with this command-line call:

```shell
matrix {device} [I2C address] [commands]
```

**Note** Again, arguments in braces `{}` are required; those in square brackets `\[\]` are optional.

* `{device}` is the path to the I2C Mini’s device file, eg. `/dev/cu.usbserial-DO029IEZ`.
* `[I2C address]` is an optional I2C address. By default, the HT16K33 uses the address `0x70`, but this can be changed.
* `[commands]` are a sequence of command blocks as described below.

| Command | Arguments | Description |
| :-: | :-: | :-- |
| `a` | [`on`\|`off`] | Activate or deactivate the display. Once activated, the matrix will remain so for as long as it is powered. Pass `on` (or `1`) to activate, `off` (or `0`) to deactivate. Calling without an argument is a *de facto* activation |
| `b` | {0-15} | Set the brightness to a value between 0 (low but not off) and 15 (high) |
| `c` | {ascii_code} [`true`\|`false`] | Plot the specified character, by code, on the display. If the second argument is included and is `true` (or `1`), the character will be centred on the display |
| `g` | {hex_values} | Plot a user-generated glyph on the display. The glyph is supplied as eight comma-separated 8-bit hex values, eg. `0x3C,0x42,0xA9,0x85,0x85,0xA9,0x42,0x3C` |
| `p` | {x} {y} [1\|0] | Plot a point as the coordinates `{x,y}`. If the third argument is `1` (or missing), the pixel will be set; if it is `0`, the pixel will be cleared |
| `t` | {string} [delay] | Scroll the specified string. The second argument is an optional delay be between column shifts in milliseconds. Default: 250ms |
| `w` | None | Clear the screen |
| `r` | {angle} | Rotate the display by the specified multiple of 90 degrees |
| `h` | None | Display help information |

#### Devices

Version 1.2.0 also supports [Pimoroni’s LED Dot Matrix Breakout boards](https://shop.pimoroni.com/products/led-dot-matrix-breakout?variant=32274405621843). These use the I2C address range 0x61-0x63, so you can select which matrix type your commands are applied to by specifying an I2C address in this range:

```
matrix /dev/cu.usbserial-DO029IEZ 0x61 a on c TS
```

The Dot Matrix Breakouts each hold two 5 x 7 LTP-305 matrices driven by a single IS31FL3730 controller for a combined area of 10 x 7. For plots and glyphs, this is the area to work with. To draw characters with the `c` command, provide two characters instead of one, as shown in the example above. The first character will be displayed on the left matrix, the second one on the right. Supported brightness values are 0 to 64.

#### Multiple Commands

Multiple commands can be issued by sequencing them at the command line. For example:

```shell
matrix /dev/cu.usbserial-DO029IEZ 0x71 w r 3 p 0 0 p 1 1 p 2 2 p 3 3 p 4 4 p 5 5 p 6 6 p 7 7
```

**Note** The client-side display buffer is not persisted across calls to `matrix`, so building up an image across calls will not work. While the display retains its own image data, the local buffer is implicitly cleared with each new call. This may be mitigated in a future release.

#### Examples

**Draw a dot at 1,1**

```
matrix /dev/cu.usbserial-DO029IEZ p 1 1
```

**Draw T, centred**

```
matrix /dev/cu.usbserial-DO029IEZ c 123 true
```

**Draw a smiley**

```
matrix /dev/cu.usbserial-DO029IEZ g 0x3C,0x42,0xA9,0x85,0x85,0xA9,0x42,0x3C
```

**Scroll “Hello, World!” across the display**

```
matrix /dev/cu.usbserial-DO029IEZ t "Hello, World!    "
```

**Note** The four spaces (two matrix columns each) ensure the text disappears of the screen at the end of the scroll.

## segment

`segment` is a specific driver for HT16K33-based 4-digit, 7-segment LEDs. It embeds `cli2c` but exposes a different, display-oriented set of commands.

You use the driver with this command-line call:

```shell
segment {device} [I2C address] [commands]
```

**Note** Again, arguments in braces `{}` are required; those in square brackets `\[\]` are optional.

* `{device}` is the path to the I2C Mini’s device file, eg. `/dev/cu.usbserial-DO029IEZ`.
* `[I2C address]` is an optional I2C address. By default, the HT16K33 uses the address `0x70`, but this can be changed.
* `[commands]` are a sequence of command blocks as described below.

| Command | Arguments | Description |
| :-: | :-: | :-- |
| `a` | [`on`\|`off`] | Activate or deactivate the display. Once activated, the matrix will remain so for as long as it is powered. Pass `on` (or `1`) to activate, `off` (or `0`) to deactivate. Calling without an argument is a *de facto* activation |
| `b` | {0-15} | Set the brightness to a value between 0 (low but not off) and 15 (high) |
| `f` | None | Flip the display vertically. Handy if your LED is mounted upside down |
| `g` | {definition} {digit} [true\|false] | Write a user-generated glyph on the display at the specified digit. The glyph is supplied as an 8-bit value comprising bits set for the segments to be lit. Optionally specify if its decimal point should be lit |
| `v` | {value} {digit} [true\|false] | Write a single-digit number (0-9, 0x0-0xF) on the display at the specified digit. Optionally specify if its decimal point should be lit |
| `c` | {char} {digit} [true\|false] | Write the character on the display at the specified digit. Only the following characters can be used: 0-9, a-f, -, space (use `' '`) and the degree symbol (used `'*'`) Optionally specify if its decimal point should be lit |
| `n` | {number} | Write a number between -999 and 9999 across the display |
| `k` | None | Light the segment’s central colon symbol |
| `w` | None | Clear the screen |
| `z` | None | Write the buffer to the screen immediately |
| `h` | None | Display help information |

Multiple commands can be issued by sequencing them at the command line. For example:

```shell
segment /dev/cu.usbserial-DO029IEZ 0x71 w f n 7777
```

**Note** The display buffer is not persisted across calls to `segment`, so building up an image across calls will not work. While the display retains its own image data, the local buffer is implicitly cleared with each new call. This may be mitigated in a future release.

#### Examples

**Draw 42.4 degrees**

```
segment /dev/cu.usbserial-DO029IEZ n 4240 d 1 c '*' 3
```

## Full Examples

The [`examples`](examples/) folder contains Python scripts that make use the above apps.

* `cpu_chart_matrix.py` — A rudimentary side-scrolling CPU activity chart. Requires an HT16K33-based 8x8 matrix LED.
* `cpu_chart_segment.py` — A CPU activity numerical percentage readout. Requires an HT16K33-based 4-digit, 7-segment matrix LED.
* `mcp9809_temp.py` — Second-by-second temperature readout. Requires an MCP98008 temperature sensor breakout.

## Acknowledgements

This work was inspired by James Bowman’s ([@jamesbowman](https://github.com/jamesbowman)) [`i2ccl` tool](https://github.com/jamesbowman/i2cdriver), which was written as a macOS/Linux/Windows command line tool to connect to his [I2CMini board](https://i2cdriver.com/mini.html).

My own I2C driver code started out based on James’ but involves numerous changes and (I think) improvements. I also removed the non-macOS code removed and some unneeded functionality that I don’t need (I2C capture, monitoring). Finally, it targets fresh firmware I wrote from the ground up to run on an RP2040-based board, not the I2CMini.

Why? Originally I was writing an HT16K33 driver based directly on James’ code, but I accidentally broke the pins off my I2CMini — only to find it is very hard to find new ones. James’ firmware is written in a modern version of Forth, so I can no choice but to learn Forth, or write code of my own. I chose the latter.

Thanks are also due to Hermann Stamm-Wilbrandt ([@Hermann-SW](https://github.com/Hermann-SW)) for the basis for the [deploy script](#deploy-the-firmware).

## Release Notes

* 1.2.0 *Unreleased*
    * A big data transfer speed improvement.
    * Support Pimoroni LTP-305 breakouts in `matrix`.
    * Document GPIO usage.
    * Add host error record.
    * Replace examples.
    * Refactor code to support future expansion.
    * Deprecate old methods.
* 1.1.1 *27 October 2022*
    * Internal changes.
* 1.1.0 *22 October 2022*
    * Support I2C bus and pin selection.
    * Support building and running client apps on Linux (RPiOS).
    * Support firmware transfer to board on Linux (RPiOS).
    * Add LED control to `cli2c`.
* 1.0.1 *18 October 2022*
    * Behind-the-scenes fixes for the client apps.
* 1.0.0 *18 October 2022*
    * Initial public release.

## Licences and Copyright

`cli2c`, `matrix` and `segment` contain small code portions © 2019, James Bowman. All other code © 2022, Tony Smith (@smittytone). They are licensed under the terms of the BSD 3-Clause Licence.

The RP2040 firmware is © 2022, Tony Smith (@smittytone). It is licensed under the terms of the MIT Licence.
