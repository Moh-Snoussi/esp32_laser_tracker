# ESP32 Face Tracker Project
## Overview
This repository hosts a project featuring an ESP32-CAM based human face tracker designed to dynamically adjust a laser pointer's position to follow a detected person.
It uses the camera, the WiFi module, and the SD card reader of the ESP32-CAM. By default, this program does not enable WiFi and the SD card reader, but you can enable them in the menuconfig.
## Configuration
Project-specific configuration can be found in `main/Kconfig.projbuild` and can be modified using the menuconfig tool.
```shell
idf.py menuconfig
```
### Optimizing for Size
To optimize for size and ensure sufficient memory for enabling WiFi and the SD card reader, follow these steps:
1. Access the menuconfig tool:
   ```shell
   idf.py menuconfig
   ```
2. Navigate to "Compiler options" -> "Optimization level" and select "Optimize for size".

### WiFi
If you want to activate WiFi, set the password and SSID in the menuconfig under "Project configuration" -> "Laser configuration" -> "Use cam server".
The ESP32-CAM will connect to the WiFi network and start a web server with an IP address provided by the router. You can view the IP address in the serial monitor.
To access the web server, open a browser on a device connected to the same network and enter the IP address. The web server will display the last image taken by the camera at an interval of 3 seconds.

### SD Card Reader
If you want to activate the SD card reader, set the option "Save detected faces in the SD card" to "y" in the menuconfig under "Project configuration" -> "Laser configuration".
When the SD card reader is activated, the ESP32-CAM will save the last image taken by the camera on the SD card.

## Flashing
To flash the project to the ESP32-CAM, follow these steps:
1. Recompile the project:
   ```shell
   idf.py build
   ```
2. Connect the ESP32-CAM to the programmer and then to the computer.
3. Flash the project:
   ```shell
   idf.py flash
   ```
4. To see the output of the program, execute:
   ```shell
   idf.py monitor
   ```
This will display the serial monitor, where you can view the program's output.

## Hardware Requirements

ESP32 CAM
Programmer (used for flashing the ESP32)
SG90 Servo
Laser pointer (attached to the servo)
Relay (used to turn the laser pointer on and off)

## Schematic
![Schematic](schema.png?raw=true "Title")


### Increase FLASH memory
idf.py menuconfig -> Component config -> ESP32-specific -> SPI Flash Size -> 16 MB

### Enable PSRAM
idf.py menuconfig -> Component config -> ESP32-specific -> Support for external, SPI-connected RAM -> Support for external, SPI-connected RAM


## Software Requirements
IDF 4.4 (Face detection will not work with IDF > 4.4)

if you have a newer version of IDF, you can downgrade to 4.4 by executing the following commands:

```shell
cd ~/esp/esp-idf
git checkout origin/release/v4.4
git submodule update --init --recursive
```

## Setup Instructions
Connect the ESP32 CAM to the programmer.
In the project directory, execute the following commands:
idf.py set-target esp32
idf.py build
idf.py flash
Connect the ESP32 CAM to the following pins:
Ground pin: GND
5V pin: 5V
GPIO 12: Servo signal pin
Relay pin: GPIO 13
