Here’s a clean and professional **README.md** you can directly upload to GitHub for your project:

---

#  STM32-Based RFID Access Control System with EEPROM Logging and Stepper Motor Automation

##  Overview

This project implements a secure RFID-based access control system using an STM32 microcontroller. It detects RFID cards, verifies user identity, logs entry/exit data into external EEPROM, and controls a stepper motor for automated access (e.g., door/gate). The system also provides UART-based data retrieval for monitoring stored records.

---

##  Features

* 📡 RFID card detection using MFRC522 module
* 🧠 User identification based on UID
* 💾 EEPROM (AT24C256) data logging
* 🕒 Real-Time Clock (RTC) timestamp recording
* 🔄 Stepper motor automation for access control
* 🖥️ UART interface to read stored data (via TeraTerm)
* 📟 LCD display for user messages
* 🔔 Buzzer indication for valid/invalid access
* 💡 LED status indication

---

##  Hardware Components

* STM32 Nucleo Board (STM32F446RET6)
* MFRC522 RFID Module
* AT24C256 EEPROM (I2C)
* Stepper Motor + Driver (ULN2003 / L298N)
* 16x2 LCD Display (HD44780 / I2C)
* Buzzer
* LEDs
* RTC (Internal STM32 RTC)
* Power Supply

---

##  System Architecture

1. RFID card is scanned using MFRC522 (SPI communication)
2. STM32 reads UID and matches with stored users
3. If valid:

   * Stepper motor rotates (door opens)
   * Entry is logged in EEPROM with timestamp
4. If invalid:

   * Buzzer alert is triggered
5. User can send command via UART ("read data")
6. Stored logs are retrieved from EEPROM and displayed in terminal

---

##  Software Requirements

* STM32CubeIDE
* Embedded C (HAL Drivers)
* TeraTerm (for UART monitoring)

---

## 📂 Project Structure

```
├── Core/
│   ├── Src/
│   │   ├── main.c
│   │   ├── stm32f4xx_hal_msp.c
│   ├── Inc/
│
├── Drivers/
│
├── RFID/
│   ├── mfrc522.c
│   ├── mfrc522.h
│
├── EEPROM/
│   ├── i2c_eeprom.c
│   ├── i2c_eeprom.h
│
├── LCD/
│
├── README.md
```

---

##  Configuration Details

* **SPI1** → RFID (MFRC522)
* **I2C1** → EEPROM (AT24C256)
* **UART2** → Serial communication (115200 baud)
* **TIM2** → Stepper/Buzzer control (if PWM used)
* **RTC** → Timestamp logging

---

## Usage Instructions

1. Power ON the system
2. Scan RFID card
3. Observe:

   * LCD displays user info
   * Stepper motor rotates (if authorized)
4. To read stored data:

   * Open TeraTerm
   * Send command:

     ```
     read data
     ```
   * EEPROM logs will be displayed

---

##  Example Output

```
CARD ID: 61 C8 71 06 NAME: PAVAN
ENTRY: 08:30:18 20-04-2026

CARD ID: F3 AB 50 06 NAME: SANDEEP
EXIT: 08:35:16 20-04-2026
```

---

##  Applications

* Smart door lock systems
* Office attendance systems
* Secure access control
* Industrial automation

---

##  Author

**Pavan Kumar Reddy V**
Embedded Systems Engineer


