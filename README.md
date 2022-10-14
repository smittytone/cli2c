# cli2c 0.1.5

An I2C driver for macOS used as the basis for an HT16K33-controlled LED matrix driver. It requires a [Raspberry Pi Pico](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html), [Adafruit QTPy RP2040](https://www.adafruit.com/product/4900), [SparkFun ProMicro RP2040](https://www.sparkfun.com/products/18288) or [Pimoroni Tiny 2040](https://shop.pimoroni.com/products/tiny-2040?variant=39560012234835) as a hardware bridge.

## Acknowledgements

This work was inspired by James Bowman’s ([@jamesbowman](https://github.com/jamesbowman)) [`i2ccl` tool](https://github.com/jamesbowman/i2cdriver), which was written as a macOS/Linux/Windows command line tool to connect to his [I2CMini board](https://i2cdriver.com/mini.html).

My own I2C driver code started out based on James’ but involves numerous changes and (I think) improvements. I also removed the non-macOS code removed and some unneeded functionality that I don’t need (I2C capture, monitoring). Finally, it targets fresh firmware I wrote from the ground up to run on an RP2040-based board, not the I2CMini.

Why? Originally I was writing an HT16K33 driver based directly on James’ code, but I accidentally broke the pins off my I2CMini — only to find it is very hard to find new ones. James’ firmware is written in a modern version of Forth, so I can no choice but to learn Forth, or write code of my own. I chose the latter.

Thanks are also due to Hermann Stamm-Wilbrandt ([@Hermann-SW](https://github.com/Hermann-SW)) for the basis for the [deploy script](#deploy-the-firmware).

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
|   |___/common                 // Code common to all versions
|
|___/examples                   // Demo apps
|   |___/cpu_chart_matrix.py    // CPU utilization display for 8x8 matrix LEDs
|   |___/cpu_chart_segment.py   // CPU utilization display for 4-digit segment LEDs
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

## cli2c

`cli2c` is a command line driver for the USB-connected RP2040-based I2C host. The host must be pre-loaded with the firmware.

It is a generic I2C driver with the following syntax:

```
cli2c {device_port} [command] ... [command]
```

Arguments in braces `{}` are required; those in square brackets `\[\]` are optional.

* `device_port` is the USB-connected I2C host’s Unix device path, eg. `/dev/cu.modem-101010`.
* [command] is an optional command block, comprising a single-character command and any required data.

| Command | Arg. Count | Args | Description |
| :-: | :-: | --- | --- |
| `w` | 2 | `address` `data_bytes` | Write the supplied data to the I2C device at `address`. `data_bytes` are comma-separated 8-bit hex values |
| `r` | 2 | `address` `count` | Read `count` bytes from the I2C device at `address` and issue an I2C STOP |
| `p` | None| Issue an I2C STOP. Usually used after one or more writes |
| `f` | 1 | {frequency} | The I2C bus frequency in multiples of 100kHz. Supported values: 1 and 4 |
| `x` | None |  | Reset the I2C bus |
| `s` | None |  |  Display devices on the I2C bus |
| `i` | None |  |  Display I2C host device information |
| `h` | None |  |  Display help information |

**Note** All message output is routed via `stderr` — data read back from an I2C device is output to `stdout` so it can be captured to a file. Returned data is currently presented as hexadecimal strings.

## matrix

`matrix` is a specific driver for HT16K33-based 8x8 LED matrices. It embeds `cli2c` but exposes a different set of commands.

You use the driver with this command-line call:

```shell
matrix {device} [I2C address] [commands]
```

Arguments in braces `{}` are required; those in square brackets `\[\]` are optional.

* `{device}` is the path to the I2C Mini’s device file, eg. `/dev/cu.usbserial-DO029IEZ`.
* `[I2C address]` is an optional I2C address. By default, the HT16K33 uses the address `0x70`, but this can be changed.
* `[commands]` are a sequence of command blocks as described below.

### Commands

These are the currently supported commands. Arguments in braces `{}` are required; those in square brackets `[]` are optional.

| Command | Arguments | Description |
| :-: | :-: | :-- |
| `-a` | [`on`\|`off`] | Activate or deactivate the display. Once activated, the matrix will remain so for as long as it is powered. Pass `on` (or `1`) to activate, `off` (or `0`) to deactivate. Calling without an argument is a *de facto* activation |
| `-b` | {0-15} | Set the brightness to a value between 0 (low but not off) and 15 (high) |
| `-c` | {ascii_code} [`true`\|`false`] | Plot the specified character, by code, on the display. If the second argument is included and is `true` (or `1`), the character will be centred on the display |
| `-g` | {hex_values} | Plot a user-generated glyph on the display. The glyph is supplied as eight comma-separated 8-bit hex values, eg. `0x3C,0x42,0xA9,0x85,0x85,0xA9,0x42,0x3C` |
| `-p` | {x} {y} [1\|0] | Plot a point as the coordinates `{x,y}`. If the third argument is `1` (or missing), the pixel will be set; if it is `0`, the pixel will be cleared |
| `-t` | {string} [delay] | Scroll the specified string. The second argument is an optional delay be between column shifts in milliseconds. Default: 250ms |
| `-w` | None | Clear the screen |
| `-r` | {angle} | Rotate the display by the specified multiple of 90 degrees |
| `-h` | None | Display help information |

Multiple commands can be issued by sequencing them at the command line. For example:

```shell
matrix /dev/cu.usbserial-DO029IEZ 0x71 -w -r 3 -p 0 0 -p 1 1 -p 2 2 -p 3 3 -p 4 4 -p 5 5 -p 6 6 -p 7 7
```

**Note** The display buffer is not persisted across calls to `matrix`, so building up an image across calls will not work. While the display retains its own image data, the local buffer is implicitly cleared with each new call. This may be mitigated in a future release.

#### Examples

**Draw a dot at 1,1**

```
matrix /dev/cu.usbserial-DO029IEZ -p 1 1
```

**Draw T, centred**

```
matrix /dev/cu.usbserial-DO029IEZ -c 123 true
```

**Draw a smiley**

```
matrix /dev/cu.usbserial-DO029IEZ -g 0x3C,0x42,0xA9,0x85,0x85,0xA9,0x42,0x3C
```

**Scroll “Hello, World!” across the display**

```
matrix /dev/cu.usbserial-DO029IEZ -t "Hello, World!    "
```

**Note** The four spaces (two matrix columns) ensure the text disappears of the screen at the end of the scroll.

## segment

`segment` is a specific driver for HT16K33-based 4-digit, 7-segment LEDs. It embeds `cli2c` but exposes a different set of commands.

You use the driver with this command-line call:

```shell
segment {device} [I2C address] [commands]
```

Arguments in braces `{}` are required; those in square brackets `\[\]` are optional.

* `{device}` is the path to the I2C Mini’s device file, eg. `/dev/cu.usbserial-DO029IEZ`.
* `[I2C address]` is an optional I2C address. By default, the HT16K33 uses the address `0x70`, but this can be changed.
* `[commands]` are a sequence of command blocks as described below.

### Commands

These are the currently supported commands. Arguments in braces `{}` are required; those in square brackets `[]` are optional.

| Command | Arguments | Description |
| :-: | :-: | :-- |
| `-a` | [`on`\|`off`] | Activate or deactivate the display. Once activated, the matrix will remain so for as long as it is powered. Pass `on` (or `1`) to activate, `off` (or `0`) to deactivate. Calling without an argument is a *de facto* activation |
| `-b` | {0-15} | Set the brightness to a value between 0 (low but not off) and 15 (high) |
| `-f` | None | Flip the display vertically. Handy if your LED is mounted upside down |
| `-g` | {definition} {digit} [true\|false] | Write a user-generated glyph on the display at the specified digit. The glyph is supplied as an 8-bit value comprising bits set for the segments to be lit. Optionally specify if its decimal point should be lit |
| `-v` | {value} {digit} [true\|false] | Write a single-digit number (0-9, 0x0-0xF) on the display at the specified digit. Optionally specify if its decimal point should be lit |
| `-c` | {char} {digit} [true\|false] | Write the character on the display at the specified digit. Only the following characters can be used: 0-9, a-f, -, space (use `' '`) and the degree symbol (used `'*'`) Optionally specify if its decimal point should be lit |
| `-n` | {number} | Write a number between -999 and 9999 across the display |
| `-k` | None | Light the segment’s central colon symbol |
| `-w` | None | Clear the screen |
| `-z` | None | Write the buffer to the screen |
| `-h` | None | Display help information |

Multiple commands can be issued by sequencing them at the command line. For example:

```shell
segment /dev/cu.usbserial-DO029IEZ 0x71 -w -f -n 7777 -z
```

Always finish with a `-z` to write the buffer to the screen.

**Note** The display buffer is not persisted across calls to `matrix`, so building up an image across calls will not work. While the display retains its own image data, the local buffer is implicitly cleared with each new call. This may be mitigated in a future release.

#### Examples

**Draw 42.4 degrees**

```
segment /dev/cu.usbserial-DO029IEZ -n 42.40 -d 1 -c '*' -z
```

## Build the Drivers

#### Xcode

You can build the code from the accompanying Xcode project. However, I use the command line tool `xcodebuild`, called from the project directory, because this makes it easier to notarise and package the binary. For more details, please see my script [`packcli.zsh`](https://github.com/smittytone/scripts/blob/main/packcli.zsh).

## Build the Firmware

1. Navigate to the repo directory.
1. Run `cmake -S . -B firmwarebuild`
1. Run `cmake --build firmwarebuild`

## Deploy the Firmware

1. Copy `firmwarebuild/firmware/pico/firmware_pico_rp2040.uf2` to a Pico in boot mode.
1. Copy `firmwarebuild/firmware/qtpy/firmware_qtpy_rp2040.uf2` to a QTPy RP2040 in boot mode.
1. Copy `firmwarebuild/firmware/qtpy/firmware_promicro.uf2` to a ProMicro RP2040 in boot mode.
1. Copy `firmwarebuild/firmware/qtpy/firmware_tiny2040.uf2` to a Tiny 2040 in boot mode.

**Note** You only need perform step 1 or 2, of course, not both.

To copy the file(s), run:

```shell
./deploy.sh /device/file /path/to/uf2
```

This will trick the RP2040-based board into booting into disk mode, then copy over the newly build firmware. When the copy completes, the RP2040 automatically reboots. This saves of a lot of tedious power-cycling with the BOOT button held down.

#### Example

```shell
./deploy.sh /dev/cu.usbserial-DO029IEZ firmwarebuild/firmware/pico/firmware_pico.uf2
```

## Licences and Copyright

`cli2c`, `matrix` and `segment` contain small code portions © 2019, James Bowman. All other code © 2022, Tony Smith (@smittytone). They are licensed under the terms of the BSD 3-Clause Licence.

The RP2040 firmware is © 2022, Tony Smith (@smittytone). It is licensed under the terms of the MIT Licence.