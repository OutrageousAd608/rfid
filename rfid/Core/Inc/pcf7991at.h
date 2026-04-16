/**
  ******************************************************************************
  * @file    pcf7991at.h
  * @brief   Header for PCF7991AT base station IC.
  * Defines driver functions for the PCF7991AT IC.
  ******************************************************************************
  */

#ifndef PCF7991AT
#define PCF7991AT

#include "main.h"

void RFID_Init(void);
void RFID_Read_Start(void);
void RFID_Read_Stop(void);
void RFID_Carrier_Off(void);
void RFID_Emulate_Raw(volatile uint32_t *timings, uint16_t length);
uint8_t RFID_Process(void);

#define SCK_H()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET)
#define SCK_L()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET)
#define MOSI_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET)
#define MOSI_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET)
#define MISO_R() HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14)

#endif // PCF7991AT