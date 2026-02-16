# STM32 MFRC522 RFID Reader

### About This Project
This project is a simple, lightweight RFID reader application for STM32 microcontrollers, built entirely using **STM32 Low Layer (LL) drivers** for maximum performance. It uses **SPI** communication to interface with an MFRC522 sensor and **USART** to output the scanned data to a serial monitor. 

The application continuously scans for MIFARE cards or keyfobs. Once a tag is detected, it reads its 4-byte Unique ID (UID) and prints it to the screen. It also features a smart anti-spam mechanism: the system automatically pauses and waits for the user to physically remove the card before starting a new scan.

### Hardware Used
* **Microcontroller:** NUCLEO-G071RB (STM32G0 series)
* **RFID Module:** MFRC522 (13.56 MHz)
<p align="center">
  <img src="https://github.com/user-attachments/assets/1a102f2f-a9b8-49c5-8360-0b1083e7be74" width="45%" />
  <img src="https://github.com/user-attachments/assets/857f954b-f846-4b79-867a-09bfb75f8cb7" width="45%" />
</p>
