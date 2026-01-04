/**
  ******************************************************************************
  * @file    rfid_driver.c
  * @brief   Implementation of low-level RFID hardware control.
  * - RX: Input Capture on PA0 (TIM2)
  * - TX: FSK Modulated PWM on PA6 (TIM3)
  ******************************************************************************
  */

#include "rfid_driver.h"
#include "tim.h"
#include "touch.h"

volatile RFID_State rfid_state;

// --- HARDWARE TUNING (STM32F401RE @ 84MHz) ---

// Frequency 0: 125.0 kHz (Idle / Logic 0)
// Calculation: 84,000,000 / 125,000 = 672 ticks. (ARR = 672 - 1)
#define RFID_ARR_125K    671 

// Frequency 1: 134.2 kHz (Active / Logic 1)
// Calculation: 84,000,000 / 134,200 = 626 ticks. (ARR = 626 - 1)
#define RFID_ARR_134K    625

// Pulse Width: 2.0 microseconds (Class-C Driver Optimization)
// Calculation: 2us * 84MHz = 168 ticks.
// This fixed width maintains stable resonance without damping the tank circuit.
#define RFID_PULSE_WIDTH 168

// --- HELPER FUNCTIONS ---

/**
 * @brief  Blocking delay using TIM2 ticks for precise emulation timing.
 * @param  ticks: Number of timer ticks (microseconds) to wait.
 */
static void delay_tim_ticks(uint32_t ticks) {
    uint32_t start = __HAL_TIM_GET_COUNTER(&htim2);
    while ((__HAL_TIM_GET_COUNTER(&htim2) - start) < ticks) {
        __NOP();
    }
}

// --- INITIALIZATION ---

void RFID_Init(void) {
    rfid_state.samples_captured = 0;
    rfid_state.is_busy = 0;
    rfid_state.data_ready = 0;

    // Ensure PWM and Capture timers are initialized but paused
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
}

// --- RECEIVER LOGIC (Analog Frontend Input) ---

void RFID_Read_Start(void) {
    if (rfid_state.is_busy) return;

    // 1. Reset Buffer State
    rfid_state.samples_captured = 0;
    rfid_state.data_ready = 0;
    rfid_state.is_busy = 1;

    // 2. Turn ON the Field
    // Passive tags require the carrier field to wake up before sending data.
    RFID_Carrier_On();

    // 3. Start Recording Edges from the Comparator
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}

void RFID_Read_Stop(void) {
    HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
    RFID_Carrier_Off();
    rfid_state.is_busy = 0;
}

// --- TRANSMITTER LOGIC (125kHz Carrier Generation) ---

void RFID_Carrier_On(void) {
    // 1. Set Base Frequency (125kHz)
    __HAL_TIM_SET_AUTORELOAD(&htim3, RFID_ARR_125K);

    // 2. Set Fixed Pulse Width (2us) for Class-C Resonance
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, RFID_PULSE_WIDTH);

    // 3. Start the PWM on PA6
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void RFID_Carrier_Off(void) {
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void RFID_Emulate_Raw(volatile uint32_t *timings, uint16_t length) {
    rfid_state.is_busy = 1;

    // 1. Prepare Timers
    HAL_TIM_Base_Start(&htim2); // Used for microsecond delays
    
    // CRITICAL: Set Carrier to 125kHz (Do not switch to 134k)
    __HAL_TIM_SET_AUTORELOAD(&htim3, RFID_ARR_125K);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, RFID_PULSE_WIDTH);

    // 2. The Infinite Loop (Until User Touches Screen)
    // We check Touch_IsPressed() to allow the user to cancel.
    while (!Touch_IsPressed()) {
        
        // --- PLAYBACK LOOP (The "Tape Recorder") ---
        for (uint16_t i = 0; i < length; i++) {
            uint32_t period = timings[i];

            // Sanity Check: Ignore noise spikes or massive silence gaps
            // (Adjust these limits if your capture data looks different)
            if (period < 100 || period > 50000) continue; 

            // --- BURST EMULATION (OOK) ---
            // ioProx expects FSK (ripples). We create ripples by 
            // toggling our 125kHz carrier ON and OFF.

            // A. Carrier ON (Loud) for 50% of the duration
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); 
            delay_tim_ticks(period / 2);

            // B. Carrier OFF (Silent) for 50% of the duration
            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
            delay_tim_ticks(period / 2);
        }

        // --- INTER-MESSAGE GAP ---
        // Real tags have a brief silence or steady carrier between repeats.
        // We leave the Carrier ON (Unmodulated) for 15ms to let the Reader sync.
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
        HAL_Delay(15); 
    }

    // 3. Cleanup (Stop everything when loop breaks)
    RFID_Carrier_Off();
    HAL_TIM_Base_Stop(&htim2);
    rfid_state.is_busy = 0;
    
    // Small delay to clear the touch press so it doesn't trigger a button on the previous menu
    HAL_Delay(300); 
}

// --- INTERRUPT HANDLER (RX) ---
// Called every time the comparator output flips (Zero Crossing detection)
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance != TIM2) return;
    if (!rfid_state.is_busy) return;

    // Safety: Prevent buffer overflow
    if (rfid_state.samples_captured >= RFID_BUFFER_SIZE) {
        RFID_Read_Stop();
        rfid_state.data_ready = 1;
        return;
    }

    // Capture the period (Time since last edge)
    uint32_t val = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    // Reset counter for the next measurement (Differential measurement)
    __HAL_TIM_SET_COUNTER(htim, 0);

    rfid_state.raw_timings[rfid_state.samples_captured] = val;
    rfid_state.samples_captured++;
}

// --- MAIN LOOP PROCESS ---
uint8_t RFID_Process(void) {
    // Check if the interrupt handler marked the capture as complete
    if (rfid_state.data_ready) return 1;

    return 0;
}