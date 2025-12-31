/**
  ******************************************************************************
  * @file    storage.h
  * @brief   Header for Flash Memory Storage.
  * Defines macros and prototypes for saving/loading signals.
  ******************************************************************************
  */

#ifndef STORAGE_H
#define STORAGE_H

#include "ui.h" // We need the Signal struct
#include "stm32f4xx_hal.h"

// --- FLASH MEMORY MAP (STM32F401RE) ---
// We use Sector 7 (Last sector) to avoid overwriting program code.
// Sector 7 is 128KB, starting at 0x08060000.
#define FLASH_STORAGE_ADDR 0x08060000 
#define FLASH_SECTOR_NUM   FLASH_SECTOR_7
#define FLASH_VOLTAGE_RANGE FLASH_VOLTAGE_RANGE_3

// --- PROTOTYPES ---

/**
 * @brief  Erases the storage sector and saves the current signal database.
 */
void Storage_SaveSignals(void);

/**
 * @brief  Reads the signal database from Flash into RAM on startup.
 */
void Storage_LoadSignals(void);

#endif // STORAGE_H