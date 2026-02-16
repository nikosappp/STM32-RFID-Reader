#include "MFRC522_STM32.h"
#include "main.h"
#include <stdio.h>

uint8_t atqa[2];

void serial_port(USART_TypeDef *usart, const char *buf) {
    while (*buf) {
        while (!LL_USART_IsActiveFlag_TXE(usart)) {}
        LL_USART_TransmitData8(usart, *buf++);
    }
    while (!LL_USART_IsActiveFlag_TC(usart)) {}
}

void MFRC522_Init(MFRC522_t *dev) {
    serial_port(USART2, "MFRC522 Min Init started\r\n");

    // Hardware reset
    LL_GPIO_ResetOutputPin(dev->rstPort, dev->rstPin);  // Pull RST pin low to reset
    LL_mDelay(50);
    LL_GPIO_SetOutputPin(dev->rstPort, dev->rstPin);    // Release RST pin high to
    LL_mDelay(50);

    //  ENABLE SPI O
    LL_SPI_Enable(dev->hspi);

    // Soft reset
    MFRC522_WriteReg(dev, PCD_CommandReg, PCD_SoftReset);
    LL_mDelay(50);

    // Clear interrupts
    MFRC522_WriteReg(dev, PCD_ComIrqReg, 0x7F);

    // Flush FIFO
    MFRC522_WriteReg(dev, PCD_FIFOLevelReg, 0x80);

    // Timer: 25ms timeout
    MFRC522_WriteReg(dev, PCD_TModeReg, 0x80);
    MFRC522_WriteReg(dev, PCD_TPrescalerReg, 0xA9);
    MFRC522_WriteReg(dev, PCD_TReloadRegH, 0x03);
    MFRC522_WriteReg(dev, PCD_TReloadRegL, 0xE8);

    // RF settings
    MFRC522_WriteReg(dev, PCD_TxAutoReg, 0x40);
    MFRC522_WriteReg(dev, PCD_RFCfgReg, 0x7F);
    MFRC522_WriteReg(dev, PCD_DemodReg, 0x4D);

    // Enable antenna
    MFRC522_AntennaOn(dev);
    LL_mDelay(10);

    uint8_t version = MFRC522_ReadReg(dev, PCD_VersionReg);
    char logbuf[64];
    if ((version == 0x91) || (version == 0x92)) {
        sprintf(logbuf, "Version: 0x%02X (counterfeit OK for UID)\r\n", version);
    } else {
        sprintf(logbuf, "Version: 0x%02X\r\n", version);
    }
    serial_port(USART2, logbuf);

    serial_port(USART2, "MFRC522 Min Init complete\r\n");
}

void MFRC522_AntennaOff(MFRC522_t *dev) {
    MFRC522_ClearBitMask(dev, PCD_TxControlReg, 0x03);
}

void MFRC522_AntennaOn(MFRC522_t *dev) {
    MFRC522_SetBitMask(dev, PCD_TxControlReg, 0x03);
}

// read one byte from rc522

uint8_t MFRC522_ReadReg(MFRC522_t *dev, uint8_t reg) {
    uint8_t addr = ((reg << 1) & 0x7E) | 0x80;  // Read bit set
    uint8_t val;

    LL_GPIO_ResetOutputPin(dev->csPort, dev->csPin);  // CS low

    //  address byte =====
    while (!LL_SPI_IsActiveFlag_TXE(dev->hspi)) {}
    LL_SPI_TransmitData8(dev->hspi, addr);

    //  RX dummy (response to address)
    while (!LL_SPI_IsActiveFlag_RXNE(dev->hspi)) {}
    (void)LL_SPI_ReceiveData8(dev->hspi);

    //  TX dummy byte (clock out register value)
    while (!LL_SPI_IsActiveFlag_TXE(dev->hspi)) {}
    LL_SPI_TransmitData8(dev->hspi, 0x00);

    //  RX register value (actual data)
    while (!LL_SPI_IsActiveFlag_RXNE(dev->hspi)) {}
    val = LL_SPI_ReceiveData8(dev->hspi);

    LL_GPIO_SetOutputPin(dev->csPort, dev->csPin);  // CS high
    LL_mDelay(1);

    return val;
}

// write one byte
void MFRC522_WriteReg(MFRC522_t *dev, uint8_t reg, uint8_t value) {
    uint8_t addr = (reg << 1) & 0x7E;  // Write bit clear

    LL_GPIO_ResetOutputPin(dev->csPort, dev->csPin);  // CS low

    //  TX address byte
    while (!LL_SPI_IsActiveFlag_TXE(dev->hspi)) {}
    LL_SPI_TransmitData8(dev->hspi, addr);

    // RX dummy
    while (!LL_SPI_IsActiveFlag_RXNE(dev->hspi)) {}
    (void)LL_SPI_ReceiveData8(dev->hspi);

    //  TX value byte
    while (!LL_SPI_IsActiveFlag_TXE(dev->hspi)) {}
    LL_SPI_TransmitData8(dev->hspi, value);

    //  RX dummy
    while (!LL_SPI_IsActiveFlag_RXNE(dev->hspi)) {}
    (void)LL_SPI_ReceiveData8(dev->hspi);

    LL_GPIO_SetOutputPin(dev->csPort, dev->csPin);  // CS high
    LL_mDelay(1);
}

void MFRC522_SetBitMask(MFRC522_t *dev, uint8_t reg, uint8_t mask) {
    uint8_t tmp = MFRC522_ReadReg(dev, reg);
    MFRC522_WriteReg(dev, reg, tmp | mask);
}

void MFRC522_ClearBitMask(MFRC522_t *dev, uint8_t reg, uint8_t mask) {
    uint8_t tmp = MFRC522_ReadReg(dev, reg);
    MFRC522_WriteReg(dev, reg, tmp & (~mask));
}

// sends reqa command and gets atqa response
uint8_t MFRC522_RequestA(MFRC522_t *dev, uint8_t *atqa) {

    MFRC522_AntennaOff(dev);  // Reset RF
    LL_mDelay(5);
    MFRC522_AntennaOn(dev);
    LL_mDelay(5);

    MFRC522_WriteReg(dev, PCD_ComIrqReg, 0x7F);      // Clear IRQs
    MFRC522_WriteReg(dev, PCD_FIFOLevelReg, 0x80);   // Flush FIFO
    MFRC522_WriteReg(dev, PCD_BitFramingReg, 0x07);  // 7 bits for REQA
    MFRC522_WriteReg(dev, PCD_FIFODataReg, PICC_REQA);
    LL_mDelay(2);

    MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Transceive);
    MFRC522_SetBitMask(dev, PCD_BitFramingReg, 0x80);      // sets the bit that triggers the transmission

    // Poll for completion (about 25ms)
    uint32_t timeout = 25;

    while (timeout > 0) {
        uint8_t status2 = MFRC522_ReadReg(dev, PCD_Status2Reg);

        if (status2 & 0x01) {  // Command complete
            uint8_t err = MFRC522_ReadReg(dev, PCD_ErrorReg);

            if (err & 0x1D) {  // Protocol/parity/buffer errors
                MFRC522_AntennaOff(dev);
                LL_mDelay(5);
                MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle); // Stop command
                return STATUS_ERROR;
            }

            uint8_t fifoLvl = MFRC522_ReadReg(dev, PCD_FIFOLevelReg);

            if (fifoLvl >= 2) {  // ATQA is 2 bytes
                atqa[0] = MFRC522_ReadReg(dev, PCD_FIFODataReg);
                atqa[1] = MFRC522_ReadReg(dev, PCD_FIFODataReg);
                MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle); // Stop command
                LL_mDelay(2);  // Post-command delay
                return STATUS_OK;
            }

            // if fifolevel < 2

            MFRC522_AntennaOff(dev);
            LL_mDelay(5);
            MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle);
            return STATUS_ERROR;
        }
        LL_mDelay(1);
        timeout--;
    }

    MFRC522_AntennaOff(dev);
    LL_mDelay(5);
    MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle);
    return STATUS_TIMEOUT;
}


uint8_t MFRC522_Anticoll(MFRC522_t *dev, uint8_t *uid) {
    char logbuf[64];

    // Start the anticollision process
    serial_port(USART2, "Anticoll\r\n");

    // Configure registers for the anticollision command
    MFRC522_WriteReg(dev, PCD_ComIrqReg, 0x7F);
    MFRC522_WriteReg(dev, PCD_FIFOLevelReg, 0x80);
    MFRC522_WriteReg(dev, PCD_BitFramingReg, 0x00);        // Send full bytes
    MFRC522_WriteReg(dev, PCD_FIFODataReg, PICC_SEL_CL1);  // Ask for UID
    MFRC522_WriteReg(dev, PCD_FIFODataReg, 0x20);          //

    LL_mDelay(2);

    // Send the command to the chip and activate transmission
    MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Transceive);
    MFRC522_SetBitMask(dev, PCD_BitFramingReg, 0x80);

    // Wait for the card's response (about 25ms)
    uint32_t timeout = 25;
    while (timeout > 0) {
        uint8_t status2 = MFRC522_ReadReg(dev, PCD_Status2Reg);

        if (status2 & 0x01) { // Command complete
            uint8_t err = MFRC522_ReadReg(dev, PCD_ErrorReg);

            // Check for protocol/parity/buffer errors
            if (err & 0x1D) {
                sprintf(logbuf, "Anticoll error: 0x%02X\r\n", err);
                serial_port(USART2, logbuf);
                MFRC522_AntennaOff(dev);
                LL_mDelay(5);
                MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle);
                return STATUS_ERROR;
            }

            //  Check if fifo has exactly 5 bytes (4-byte UID + 1-byte BCC)
            uint8_t fifoLvl = MFRC522_ReadReg(dev, PCD_FIFOLevelReg);

            if (fifoLvl == 5) {
                // Read the 5 bytes from the FIFO buffer
                for (int i = 0; i < 5; i++) {
                    uid[i] = MFRC522_ReadReg(dev, PCD_FIFODataReg);
                }

                // Validate BCC (Block Check Character) for data integrity
                uint8_t calcBcc = uid[0] ^ uid[1] ^ uid[2] ^ uid[3];
                if (uid[4] != calcBcc) {
                    sprintf(logbuf, "Anticoll bad BCC: calc=0x%02X, got=0x%02X\r\n", calcBcc, uid[4]);
                    serial_port(USART2, logbuf);
                    MFRC522_AntennaOff(dev);
                    LL_mDelay(5);
                    MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle);
                    return STATUS_ERROR;
                }

                // Success... Print the raw UID and BCC
                sprintf(logbuf, "Anticoll UID: %02X %02X %02X %02X %02X\r\n", uid[0], uid[1], uid[2], uid[3], uid[4]);
                serial_port(USART2, logbuf);

                MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle); // Stop the Transceive command
                LL_mDelay(2);
                return STATUS_OK;
            }
            // If fifoLvl < 5

            sprintf(logbuf, "Anticoll wrong FIFO size\r\n");
            serial_port(USART2, logbuf);
            MFRC522_AntennaOff(dev);
            LL_mDelay(5);
            MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle);
            return STATUS_ERROR;
        }
        LL_mDelay(1);
        timeout--;
    }


    serial_port(USART2, "Anticoll timeout\r\n");
    MFRC522_AntennaOff(dev);
    LL_mDelay(5);
    MFRC522_WriteReg(dev, PCD_CommandReg, PCD_Idle);
    return STATUS_TIMEOUT;
}


// Extracts only the 4 bytes of the UID

uint8_t MFRC522_ReadUid(MFRC522_t *dev, uint8_t *uid) {

	char logbuf[64];
	serial_port(USART2, "Reading UID...\r\n");

	uint8_t rawUid[5];
	if (MFRC522_Anticoll(dev, rawUid) != STATUS_OK) {
		serial_port(USART2, "Anticollision failed\r\n");
		return STATUS_ERROR;
	}

	for(int i = 0; i < 4; i++){
		uid[i] = rawUid[i];
	}

	sprintf(logbuf, "Card UID: %02X %02X %02X %02X\r\n", uid[0], uid[1], uid[2], uid[3] );
	serial_port(USART2, logbuf);
	return STATUS_OK;
}


uint8_t waitcardRemoval(MFRC522_t *dev) {
    char logbuf[48];
    sprintf(logbuf, "Waiting for card removal...\r\n");
    serial_port(USART2, logbuf);

    while (1) {
        if (MFRC522_RequestA(dev, atqa) != STATUS_OK) {
            serial_port(USART2, "Card removed\r\n");
            return STATUS_OK;
        }
        LL_mDelay(100);
    }
}

uint8_t waitcardDetect(MFRC522_t *dev) {
    atqa[0] = atqa[1] = 0;
    char logbuf[48];
    sprintf(logbuf, "Waiting for the card...\r\n");
    serial_port(USART2, logbuf);

    while (1) {
        if (MFRC522_RequestA(dev, atqa) == STATUS_OK) {
            serial_port(USART2, "Card detected\r\n");
            return STATUS_OK;
        }
        LL_mDelay(100);
    }
}
