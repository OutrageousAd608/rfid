/**
  ******************************************************************************
  * @file    storage.c
  * @brief   Implementation of persistent storage using internal Flash.
  ******************************************************************************
  */

#include "storage.h"
#include <string.h>

// Total size needed for the database
#define DB_SIZE (sizeof(Signal) * MAX_SLOTS)

void Storage_SaveSignals(void) {
    HAL_FLASH_Unlock();

    // 1. Erase the Sector
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE;
    EraseInitStruct.Sector = FLASH_SECTOR_NUM;
    EraseInitStruct.NbSectors = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        return; // Failed to erase
    }

    // 2. Program Data (Byte by Byte)
    // Note: With larger RFID buffers, this loop will take longer.
    uint8_t *data_ptr = (uint8_t *)signal_db;
    uint32_t address = FLASH_STORAGE_ADDR;

    for (int i = 0; i < DB_SIZE; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address + i, data_ptr[i]);
    }

    HAL_FLASH_Lock();
}

void Storage_LoadSignals(void) {
    // 1. Point to the Flash address
    Signal *flash_data = (Signal *)FLASH_STORAGE_ADDR;

    // 2. Copy data to RAM
    memcpy(signal_db, flash_data, DB_SIZE);

    // 3. Sanity Check
    // If flash was empty (0xFF), mark slots as inactive.
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (signal_db[i].is_active == 0xFF) {
            signal_db[i].is_active = 0;
            memset(signal_db[i].name, 0, NAME_LEN);
        }
    }
}