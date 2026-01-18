#include <stdio.h>
extern int stdout_init (void);
#include <stdlib.h>
#include "Driver_I2C.h"

/* I2C Driver instance - using I2C0 based on your driver code */
extern ARM_DRIVER_I2C Driver_I2C3;
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C3;

/* I2C device address (7-bit) - change this to match your device */
#define I2C_DEVICE_ADDR  0x50

/* Callback function for I2C events */
static void I2C_Callback(uint32_t event)
{
}

/* Simple delay function */
static void delay(void)
{
    for (volatile int i = 0; i < 1000000; i++);
}

int main(void)
{
    stdout_init();
    int32_t status;
    uint8_t tx_buffer[2];

    printf("I2C Master Demo Started\n");

    /* Initialize I2C driver */
    status = I2Cdrv->Initialize(I2C_Callback);
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);

    tx_buffer[0] = 0x00;  /* Register address */
    tx_buffer[1] = 0xAA;  /* Data byte 1 */
    printf("Sending data to I2C device at address 0x%02X\n", I2C_DEVICE_ADDR);

    status = I2Cdrv->MasterTransmit(I2C_DEVICE_ADDR, tx_buffer, 2, false);
    if (status != ARM_DRIVER_OK) {
        printf("I2C Master Transmit failed: %d\n", status);
        return -1;
    }

    /* Wait for transfer to complete */
    delay();

    int32_t data_count = I2Cdrv->GetDataCount();
    printf("Bytes transmitted: %d\n", data_count);


    return 0;
}
