/**
  ******************************************************************************
  * @file    rf_driver.h
  * @brief   Header for RF Signal Capture & Replay Driver (433MHz).
  * Production Version.
  ******************************************************************************
  */

#ifndef RF_DRIVER_H
#define RF_DRIVER_H

#include "main.h"

// --- CAPTURE CONFIGURATION ---

// Maximum number of transitions (edges) to record.
// 512 edges is standard for capturing rolling code remotes (~256 bits).
#define RF_BUFFER_SIZE  512 

// Minimum pulse width in microseconds to be considered valid.
// Acts as a digital filter to ignore high-frequency RF noise (< 50us).
#define MIN_PULSE_LEN   50 

// Silence timeout in microseconds to detect the end of a packet.
// 20ms (20000us) is the standard gap between packets for ASK/OOK key fobs.
#define RF_SILENCE_TIMEOUT 20000

// --- TRANSMITTER PIN DEFINITION ---
// ! IMPORTANT ! Ensure these match your STM32CubeMX (.ioc) configuration.
#define RF_TX_GPIO_PORT GPIOA
#define RF_TX_PIN       GPIO_PIN_1

// --- SIGNAL STRUCTURE ---
typedef struct {
    uint32_t timings[RF_BUFFER_SIZE]; // Buffer storing pulse durations (us)
    uint16_t count;                   // Total number of edges captured
    uint8_t  is_capturing;            // Status Flag: 1 = Busy, 0 = Idle
    uint8_t  capture_complete;        // Status Flag: 1 = Data Ready
} RF_Capture_State;

// Global Instance (Volatile ensures safe access between ISR and Main Loop)
extern volatile RF_Capture_State rf_rx;

// --- PROTOTYPES ---

/**
 * @brief  Initializes the driver structures.
 */
void RF_Init(void);

/**
 * @brief  Resets buffer and enables hardware interrupts to start recording.
 */
void RF_Start_Capture(void);

/**
 * @brief  Stops hardware interrupts and finalizes capture state.
 */
void RF_Stop_Capture(void);

/**
 * @brief  Replays the recorded signal through the TX pin.
 * This is a blocking function using microsecond delays.
 * @param  timings: Array of pulse durations.
 * @param  length: Number of pulses to replay.
 */
void RF_Transmit(volatile uint32_t *timings, uint16_t length);

/**
 * @brief  Interrupt Handler Logic.
 * Calculates pulse width and stores it in the buffer.
 */
void RF_Handle_IRQ(TIM_HandleTypeDef *htim);

/**
 * @brief  State Machine Processor.
 * Checks for signal timeout (silence) to determine if capture is done.
 * @return 1 if a new signal is ready to save, 0 otherwise.
 */
uint8_t RF_Process(void);

#endif // RF_DRIVER_H