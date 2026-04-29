# STM32-RFID Access Control System

## Overview
STM32-based access control that authenticates RFID cards,
logs events in EEPROM, and controls a servo motor.

## Hardware Used
- STM32F103 microcontroller
- RC522 RFID reader (SPI)
- I2C EEPROM (AT24C256)
- SG90 Servo motor
- 16x2 LCD display

## Technologies
Embedded C | STM32CubeIDE | SPI | I2C | UART | GPIO

## How It Works
1. RFID card is scanned via RC522 over SPI
2. STM32 verifies card ID against stored records in EEPROM
3. Servo motor opens door on successful authentication
4. All events logged with timestamp in EEPROM
