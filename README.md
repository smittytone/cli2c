# cli2c

An experimental I2C driver for macOS. Requires a QTPy RP2040 as a hardware bridge.


## What’s What

The contents of this repo are:

```
/cli2c
|
|___/cli2c                  // The macOS driver and related code
|   |___/cli2c              // The basic CLI driver
|   |___/matrix             // An HT16K33-oriented version
|   |___/common             // Code common to all versions
|
|___/firmware               // The RP2040 host firmware, written in C
|   |___/pico               // The Raspberry Pi Pico version
|   |___/qtpy               // An Adafruit QTPy RP2040 version
|   |___/common             // Code common to all versions
|
|___CMakeLists.txt          // Top-level firmware project CMake config file
|___pico_sdk_import.cmake   // Raspberry Pi Pico SDK CMake import script
|
|___firmware.code-workspace // Visual Studio Code workspace for firmware
|___cli2c.xcodeproj         // Xcode workspace for cli2c
|
|___README.md
|___LICENSE.md
```
