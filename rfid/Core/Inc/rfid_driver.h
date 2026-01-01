/**
  ******************************************************************************
  * @file    rfid_driver.h
  * @brief   Driver for Custom 125kHz RFID Analog Frontend.
  * - RX: Uses Input Capture (TIM2) to measure FSK signal periods.
  * - TX: Uses PWM (TIM3) to generate 125kHz carrier field.
  ******************************************************************************
  */

#ifndef RFID_DRIVER_H
#define RFID_DRIVER_H

#include "main.h"

// --- CONFIGURATION ---

// Buffer size for raw signal edges.
// FSK signals have many transitions, so we allocate a larger buffer than simple ASK.
#define RFID_BUFFER_SIZE  2048

// The carrier frequency in Hz (Standard RFID).
#define RFID_CARRIER_FREQ 125000

// --- PIN DEFINITIONS (Must match PCB Design) ---
// RX Pin: Output of Comparator/OpAmp -> STM32 Timer Channel.
#define RFID_RX_PIN       GPIO_PIN_0
#define RFID_RX_PORT      GPIOA

// TX Pin: MOSFET Driver Gate -> STM32 Timer PWM Output.
#define RFID_TX_PIN       GPIO_PIN_6
#define RFID_TX_PORT      GPIOA

// --- DATA STRUCTURE ---
typedef struct {
    uint32_t raw_timings[RFID_BUFFER_SIZE]; // Stores period lengths (for FSK analysis)
    uint16_t samples_captured;              // Total edges recorded
    uint8_t  is_busy;                       // 1 = Reading/Emulating, 0 = Idle
    uint8_t  data_ready;                    // 1 = Buffer full or capture complete
} RFID_State;

extern volatile RFID_State rfid_state;

// --- PUBLIC FUNCTIONS ---

/**
 * @brief  Initializes Timer2 (Input Capture) and Timer3 (PWM Output).
 */
void RFID_Init(void);

/**
 * @brief  Starts listening to the analog comparator output.
 * Captures the period of incoming waves to detect FSK shifts.
 * Automatically enables the Carrier Field to power passive tags.
 */
void RFID_Read_Start(void);

/**
 * @brief  Stops listening, disables interrupts, and turns off the carrier.
 */
void RFID_Read_Stop(void);

/**
 * @brief  Starts generating the 125kHz carrier wave.
 * Used during Reading (to power tags) and Emulation.
 */
void RFID_Carrier_On(void);

/**
 * @brief  Stops the carrier wave to save power.
 */
void RFID_Carrier_Off(void);

/**
 * @brief  Replays the captured signal.
 * Modulates the carrier field based on saved timings.
 * @param  timings: Array of period durations.
 * @param  length: Number of samples to replay.
 */
void RFID_Emulate_Raw(volatile uint32_t *timings, uint16_t length);

/**
 * @brief  Main loop processor. Handles timeouts and buffer management.
 * @return 1 if a new signal was successfully captured, 0 otherwise.
 */
uint8_t RFID_Process(void);

#endif // RFID_DRIVER_H
