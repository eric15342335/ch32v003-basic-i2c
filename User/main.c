/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *7-bit addressing mode, master/slave mode, transceiver routine:
 *I2C1_SCL(PC2)\I2C1_SDA(PC1).
 *This routine demonstrates that Master sends and Slave receives.
 *Note: The two boards download the Master and Slave programs respectively,
 *    and power on at the same time.
 *      Hardware connection:
 *            MCU 1    MCU 2
 *            [PC2]-----[PC2  PD2] (Internal resistor)
 *                   |_________|
 *
 *            [PC1]-----[PC1  PD1] (Internal resistor)
 *                   |_________|
 *
 */

#include "debug.h"

/* I2C Mode Definition */
#define HOST_MODE 0
#define SLAVE_MODE 1

/* I2C Communication Mode Selection */
//#define I2C_MODE   HOST_MODE
#define I2C_MODE SLAVE_MODE

/* Global define */
#define Size 6
#define RXAdderss 0x02
#define TxAdderss 0x02

/* Global Variable */
volatile u8 TxData[Size] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
volatile u8 RxData[5][Size];

/*********************************************************************
 * @fn      IIC_Init
 *
 * @brief   Initializes the IIC peripheral.
 *
 * @return  none
 */
void IIC_Init(u32 bound, u16 address) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitTSturcture = {0};
    // RCC_APB2Periph_AFIO
    RCC_APB2PeriphClockCmd(
            RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_AFIO,
            ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // internal resistor, why need to use? tldr for i2c, for more info, see
    // https://www.analog.com/en/resources/technical-articles/i2c-primer-what-is-i2c-part-1.html
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);


    I2C_InitTSturcture.I2C_ClockSpeed = bound;
    I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitTSturcture.I2C_OwnAddress1 = address;
    I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitTSturcture);

    I2C_Cmd(I2C1, ENABLE);
}
#define I2C_SPEED 80000
/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void) {
    u8 i = 0;
    u8 j = 0;
    u8 p = 0;
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("SystemClk:%d\r\n", SystemCoreClock);
    Delay_Ms(1000);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());

#if (I2C_MODE == HOST_MODE)
    printf("IIC Host mode\r\n");
    IIC_Init(I2C_SPEED, TxAdderss);

    for (j = 0; j < 5; j++) {
        while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

        I2C_GenerateSTART(I2C1, ENABLE);
        printf("I2C generated start!\n");

        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
            ;
        printf("I2C Event master transmitter mode selected!\n");
        I2C_Send7bitAddress(I2C1, 0x02, I2C_Direction_Transmitter);
        printf("I2c Sent 7 bit Address\n");

        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
            ;
        printf("I2C Event master transmitter mode not selected!\n");
        for (i = 0; i < 6; i++) {
            if (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) != RESET) {
                Delay_Ms(100);
                printf("Sent %d\r\n", TxData[i]);
                I2C_SendData(I2C1, TxData[i]);
            }
        }
        printf("Sending finished!\r\n");
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
            ;
        printf("I2C Event master byte no longer transmitted!\r\n");
        I2C_GenerateSTOP(I2C1, ENABLE);
        printf("I2C Generated STOP!\r\n");
        Delay_Ms(1000);
    }

#elif (I2C_MODE == SLAVE_MODE)
    printf("IIC Slave mode\r\n");
    IIC_Init(I2C_SPEED, RXAdderss);

    for (p = 0; p < 5; p++) {
        i = 0;
        printf("Waiting for receiver address match!\n");
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED))
            ;
        printf("I2C Event slave receiver address match disappeared!\n");
        while (i < 6) {
            if (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) != RESET) {
                RxData[p][i] = I2C_ReceiveData(I2C1);
                printf("Received! %d\r\n", RxData[p][i]);
                i++;
            }
        }
        I2C1->CTLR1 &= I2C1->CTLR1;
    }
    printf("RxData:\r\n");
    for (p = 0; p < 5; p++) {
        for (i = 0; i < 6; i++) {
            printf("%02x ", RxData[p][i]);
        }
        printf("\r\n ");
    }

#endif

    while (1)
        ;
}
