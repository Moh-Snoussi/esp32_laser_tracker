# ESP32 Face Tracker Project
This repository hosts a project featuring an ESP32-based human face tracker designed to dynamically adjust a laser pointer's position to follow a detected person.

## Hardware Requirements
ESP32 CAM
Programmer (used for flashing the ESP32)
SG90 Servo
Laser pointer (attached to the servo)

## Schematic
![Schematic](schema.png?raw=true "Title")


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
