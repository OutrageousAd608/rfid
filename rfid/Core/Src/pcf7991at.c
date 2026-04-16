/**
 ******************************************************************************
* @file    pcf7991at.c
* @brief   Implementation of driver code for the PCF7991AT base station IC.
******************************************************************************
*/

#include "pcf7991at.h"

void PCF_Delay(uint32_t count) {

    for(volatile uint32_t i = 0; i < count; i++) {

        __NOP();

    }

}

void PCF_InitSync(void) {

    SCK_H();
    PCF_Delay(1000);

    MOSI_H();
    PCF_Delay(1000);

    SCK_L();
    PCF_Delay(1000);

}

void PCF_Write(uint8_t cmd){

	PCF_InitSync();

    for (int i = 7; i >= 0; i--) {

        if (cmd & (1 << i)) { MOSI_H(); } else { MOSI_L(); }

        PCF_Delay(1000);
        SCK_H();
        PCF_Delay(1000);
        SCK_L();
        PCF_Delay(1000);

    }

    MOSI_L();

}

uint8_t PCF_WriteRead(uint8_t cmd) {

	PCF_InitSync();

    uint8_t response = 0;

    for (int i = 7; i >= 0; i--) {

        if (cmd & (1 << i)) { MOSI_H(); } else { MOSI_L(); }

        PCF_Delay(1000);
        SCK_H();
        PCF_Delay(1000);
        SCK_L();
        PCF_Delay(1000);

    }

    MOSI_L();
    PCF_Delay(1000);

    for (int i = 7; i >= 0; i--) {

        SCK_H();
        PCF_Delay(1000);

        if (MISO_R() == GPIO_PIN_SET) {

            response |= (1 << i);

        }

        SCK_L();
        PCF_Delay(1000);

    }

    return response;

}

void RFID_Init(void){

    //turn off 5V square wave immediately
    PCF_Write(0b01010001);

    //configure pages with general parameters
    //configure page 0
    PCF_Write(0b01000011);
    //configure page 1
    PCF_Write(0b01010011);
    //configure page 2
    PCF_Write(0b01100000);
    //configure page 3
    PCF_Write(0b01111000);

    //turning on 5V square waves
    PCF_Write(0b01010010);
    HAL_Delay(100);

    //storing ACQAMP
    PCF_Write(0b01100100);
    HAL_Delay(10);
    PCF_Write(0b01100000);

    //reading phase
    uint8_t phase = PCF_WriteRead(0b00001000);
    phase &= 0b00111111;
    phase *= 2;
    if(phase > 0b00111111){
        phase = 0b00111111;
    }

    //write the sampling time
    PCF_Write(0b10000000 + phase);

    //turning off 5V square wave again
    PCF_Write(0b01010011);

}

void RFID_Read_Start(void){

    PCF_Write(0b01010010);
    PCF_Write(0b11100000);

}

void RFID_Read_Stop(void){

    SCK_L();
    SCK_H();
    PCF_Write(0b01010011);

}

void RFID_Carrier_Off(void){

}

void RFID_Emulate_Raw(volatile uint32_t *timings, uint16_t length){

}

uint8_t RFID_Process(void){
    
    return 0;

}
