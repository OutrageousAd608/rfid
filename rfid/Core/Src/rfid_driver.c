/**
  ******************************************************************************
  * @file    rfid_driver.c
  * @brief   Implementation of low-level RFID hardware control.
  ******************************************************************************
  */

#include "rfid_driver.h"
#include "tim.h" // Ensure TIM2 (RX) and TIM3 (TX) are configured in .ioc

volatile RFID_State rfid_state;

// --- HELPER FUNCTIONS ---

/**
 * @brief  Blocking delay using TIM2 ticks for precise emulation timing.
 * @param  ticks: Number of timer ticks (usually microseconds) to wait.
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
    // Starts the 125kHz Square Wave on PA6 (TIM3 CH1)
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void RFID_Carrier_Off(void) {
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}

void RFID_Emulate_Raw(volatile uint32_t *timings, uint16_t length) {
    rfid_state.is_busy = 1;

    // Ensure Base Timer is running for our delay function
    HAL_TIM_Base_Start(&htim2);

    for (uint16_t i = 0; i < length; i++) {
        // Simple Modulation Logic (ASK Example)
        // For FSK, this logic would adjust the PWM Prescaler instead of ON/OFF.

        if (i % 2 == 0) {
            RFID_Carrier_On();
        } else {
            RFID_Carrier_Off();
        }

        delay_tim_ticks(timings[i]);
    }

    // Cleanup
    RFID_Carrier_Off();
    rfid_state.is_busy = 0;
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
    // This value represents the Frequency.
    // Short period = Logic 0, Long period = Logic 1 (for FSK decoding).
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

    // (Optional) Add silence timeout logic here if needed

    return 0;
}
