# rt-thread_ex_2025
How to build PlatformIO based project
=====================================

1. [Install PlatformIO Core](https://docs.platformio.org/page/core.html)
2. Download [development platform with examples](https://github.com/platformio/platform-chipsalliance/archive/develop.zip)
3. Extract ZIP archive
4. Run these commands:

```shell
# Change directory to example
$ cd platform-chipsalliance/examples/rtosal-freertos

# Build project
$ pio run

# Upload firmware
$ pio run --target upload

# Build specific environment
$ pio run -e swervolf_nexys

# Upload firmware for the specific environment
$ pio run -e swervolf_nexys --target upload

# Clean build files
$ pio run --target clean
```
