# cli2c

An I2C driver for macOS used as the basis for an HT16K33-controlled LED matrix driver. It requires a Raspberry Pi Pico or QTPy RP2040 as a hardware bridge.

## Acknowledgements

This work was inspired by James Bowman’s ([@jamesbowman](https://github.com/jamesbowman)) [`i2ccl` tool](https://github.com/jamesbowman/i2cdriver), which was written as a macOS/Linux/Windows command line tool to connect to his I2CMini board.

My own I2C driver code is based on James’ but with a number of (I think) improvements, the non-macOS code removed, some unneeded functionality (I2C capture, monitoring) I don’t need, and the target not an I2C board, but firmware I wrote from the ground up to run on an RP2040-based board.

Why? Originally I was writing an HT16K33 driver based directly on James’ code, but I accidentally broke the pins off my I2CMini — only to find it is very hard to find new ones. So I adapted it for a Mac-RP2040 combo.

And thanks are due to Hermann Stamm-Wilbrandt ([@Hermann-SW](https://github.com/Hermann-SW)) for the basis for the [deploy script](#deploy-the-firmware).

## What’s What

The contents of this repo are:

```
/cli2c
|
|___/cli2c                  // The macOS driver and related code
|   |___/cli2c              // The basic CLI driver
|   |___/matrix             // An HT16K33 8x8 matrix-oriented version of the driver
|   |___/segment            // An HT16K33 4-digit, 7-segment-oriented version of the driver
|   |___/common             // Code common to all versions
|
|___/firmware               // The RP2040 host firmware, written in C
|   |___/pico               // The Raspberry Pi Pico version
|   |___/qtpy               // An Adafruit QTPy RP2040 version
|   |___/common             // Code common to all versions
|
|___/examples               // Demo apps
|   |___/cpu_chart.py       // CPU utilization display for matrices
|
|___CMakeLists.txt          // Top-level firmware project CMake config file
|___pico_sdk_import.cmake   // Raspberry Pi Pico SDK CMake import script
|
|___firmware.code-workspace // Visual Studio Code workspace for firmware
|___cli2c.xcodeproj         // Xcode workspace for cli2c
|
|___deploy.sh               // A .uf2 deployment script that saves pressing 
|                           // RESET/BOOTSEL buttons.
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

Arguments in braces `{}` are required; those in square brackets `[]` are optional.

* `device_port` is the USB-connected I2C host’s Unix device path, eg. `/dev/cu.modem-101010`.
* [command] is an optional command block, comprising a single-character command and any required data.

| Command | Arg. Count | Args | Description |
| :-: | :-: | --- | --- |
| `w` | 2 | `address` `data_bytes` | Write the supplied data to the I2C device at `address`. `data_bytes` are comma-separated 8-bit hex values |
| `r` | 2 | `address` `count` | Read `count` bytes from the I2C device at `address` and issue an I2C STOP |
| `p` | 0 | Issue an I2C STOP |
| `d` | 0 | Display devices on the I2C bus |

## matrix

`matrix` is a specific driver for HT16K33-based 8x8 LED matrices. It embeds `cli2c` but exposes a different set of commands.

You use the driver with this command-line call:

```shell
matrix {device} [I2C address] [commands]
```

Arguments in braces `{}` are required; those in square brackets `[]` are optional.

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

Multiple commands can be issued by sequencing them at the command line. For example:

```shell
matrix /dev/cu.usbserial-DO029IEZ 0x71 -w -r 3 -p 0 0 -p 1 1 -p 2 2 -p 3 3 -p 4 4 -p 5 5 -p 6 6 -p 7 7
```

You should note that the display buffer is not persisted across calls to `matrix`, so building up an image across calls will not work. The display is implicitly cleared with each new call.

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

Arguments in braces `{}` are required; those in square brackets `[]` are optional.

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
| `-n` | {number} | Write a number between -999 and 9999 across the display |
| `-t` | {string} [delay] | Scroll the specified string. The second argument is an optional delay be between column shifts in milliseconds. Default: 250ms |
| `-w` | None | Clear the screen |

Multiple commands can be issued by sequencing them at the command line. For example:

```shell
segment /dev/cu.usbserial-DO029IEZ 0x71 -w -f -n 7777
```

## Build the Drivers

#### Xcode

You can build the code from the accompanying Xcode project. However, I use the command line tool `xcodebuild`, called from the project directory, because this makes it easier to notarise and package the binary. For more details, please see my script [`packcli.zsh`](https://github.com/smittytone/scripts/blob/main/packcli.zsh).

## Build the Firmware

1. Navigate to the repo directory.
1. Run `cmake -S . -B firmwarebuild`
1. Run `cmake --build firmwarebuild`

## Deploy the Firmware

1. Copy `firmwarebuild/firmware/qtpy/firmware_qtpy_rp2040.uf2` to a QTPy RP2040 in boot mode.
1. Copy `firmwarebuild/firmware/pico/firmware_pico_rp2040.uf2` to a Pico in boot mode.

**Note** You only need perform step 1 or 2, of course, not both.

To copy the file(s), run:

```shell
./deploy.sh /device/file /path/to/uf2
```

This will trick the RP2040-based board into booting into disk mode, then copy over the newly build firmware. When the copy completes, the RP2040 automatically reboots.

#### Example

```shell
./deploy.sh /dev/cu.usbserial-DO029IEZ firmwarebuild/firmware/pico/firmware_pico.uf2
```

## Licences and Copyright

`cli2c`, `matrix` and `segment` contain code portions © 2019, James Bowman. They are licensed under the terms of the BSD 3-Clause Licence.

The RP2040 firmware is © 2022, Tony Smith (@smittytone). It is licensed under the terms of the MIT Licence.