#include <stdio.h>
#include <string.h>
#include "Driver_I2C.h"

extern int stdout_init (void);
extern ARM_DRIVER_I2C Driver_I2C3;
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C3;

// Configuration & Protocol Definitions
// ====================================

/* I2C Address */
#define I2C_ADDR_SSD0303        0x3C

/* Protocol Control Bytes */
#define CTRL_CMD                0x80
#define CTRL_DATA               0x40

/* SSD0303 Command Set */
#define CMD_SET_COL_LOW         0x00 /* 0x00 - 0x0F */
#define CMD_SET_COL_HIGH        0x10 /* 0x10 - 0x1F */
#define CMD_SET_PAGE            0xB0 /* 0xB0 - 0xB7 */
#define CMD_DISPLAY_ON          0xAF
#define CMD_DISPLAY_OFF         0xAE
#define CMD_INVERSE             0xA7

/* Masks for variable commands */
#define MASK_COL_LOW            0x0F
#define MASK_COL_HIGH           0x0F
#define MASK_PAGE               0x07

/* Screen Geometry (VersatilePB/QEMU Specific) */
#define SCREEN_VISIBLE_OFFSET   36
#define SCREEN_PAGE_COUNT       2
#define SCREEN_COL_COUNT        96

// Helper function
// ====================================
void rotate_bitmap(const uint8_t pattern[8], uint8_t result[8], int rotation) {
    // Normalize rotation to 0-3
    rotation = rotation & 0x03;
    if(rotation == 0) {
        memcpy(result, pattern, 8);
        return;
    }
    memset(result, 0x00, 8);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            switch(rotation) {
                case 1:
                    if (pattern[row] & (1 << col)) {
                        result[col] |= (1 << (7 - row));
                    }
                    break;
                case 2:
                    if (pattern[row] & (1 << col)) {
                        result[7 - row] |= (1 << (7 - col));
                    }
                    break;
                case 3:
                    if (pattern[row] & (1 << col)) {
                        result[7 - col] |= (1 << row);
                    }
                    break;
            }
        }
    }

}

void delay(void) {
    for (volatile int i = 0; i < 100000; i++);
}

// OLED Wrapper
// ====================================
int oled_cmd(uint8_t cmd) {
    uint8_t buf[2];
    buf[0] = CTRL_CMD;
    buf[1] = cmd;
    return I2Cdrv->MasterTransmit(I2C_ADDR_SSD0303, buf, 2, false);
}

/* Set the cursor position in specific Page (row 0-7) and Column */
// page: 0-1, col 0-96, each tuple is a 8*8 rect
void oled_set_cursor(uint8_t page, uint8_t col) {
    // Add the hardware offset so '0' is actually the start of the visible screen
    uint8_t real_col = col + SCREEN_VISIBLE_OFFSET;

    // Use defines to construct the command bytes
    oled_cmd(CMD_SET_PAGE     | (page     & MASK_PAGE));     // Set Page Address
    oled_cmd(CMD_SET_COL_LOW  | (real_col & MASK_COL_LOW));  // Set Lower Column Address
    oled_cmd(CMD_SET_COL_HIGH | ((real_col >> 4) & MASK_COL_HIGH)); // Set Higher Column Address
}

/* Draw a bitmap pattern */
void oled_draw_pattern(int page, int col) {
    // 8x8 Block
    static const uint8_t pattern[8] = {
        0b00111100,
        0b01000010,
        0b10000101,
        0b10001001,
        0b10000001,
        0b10000001,
        0b01000010,
        0b00111100,
    };

    uint8_t rotated[8];
    rotate_bitmap(pattern, rotated, 3);

    uint8_t data_stream[1 + sizeof(rotated)];

    // 1. Position cursor at page, col
    oled_set_cursor(page, col);

    // 2. Prepare Data Stream
    data_stream[0] = CTRL_DATA;
    memcpy(&data_stream[1], rotated, sizeof(rotated));

    // 3. Transmit
    I2Cdrv->MasterTransmit(I2C_ADDR_SSD0303, data_stream, sizeof(data_stream), false);
}

int main(void) {
    stdout_init();
    printf("[I2C] SSD0303 OLED Demo\n");

    /* Initialize Driver */
    I2Cdrv->Initialize(NULL);
    I2Cdrv->PowerControl(ARM_POWER_FULL);
    I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);

    printf("[I2C] Turning Display ON...\n");
    oled_cmd(CMD_DISPLAY_ON);
    oled_cmd(CMD_INVERSE);

    printf("[I2C] Drawing Smiley...\n");
    oled_draw_pattern(0, 0);
    oled_draw_pattern(1, 96 - 8);

    printf("[I2C] Done.\n");
    return 0;
}
