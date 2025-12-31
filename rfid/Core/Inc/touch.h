/**
  ******************************************************************************
  * @file    touch.h
  * @brief   Header for XPT2046 Resistive Touch Controller.
  * Handles raw ADC readings and coordinate calibration.
  ******************************************************************************
  */

#ifndef TOUCH_H
#define TOUCH_H

#include "main.h"

// --- CALIBRATION DATA ---
// Maps raw 12-bit ADC readings to screen pixel coordinates.
#define RAW_X_MIN  200
#define RAW_X_MAX  3700
#define RAW_Y_MIN  300
#define RAW_Y_MAX  3800

// --- UI STRUCTURES ---
typedef struct {
    uint16_t x;       // Top-left X position
    uint16_t y;       // Top-left Y position
    uint16_t width;   // Width of the button
    uint16_t height;  // Height of the button
} ButtonDef;

// --- PROTOTYPES ---

/**
 * @brief  Checks the IRQ pin to see if the screen is currently being touched.
 * @return 1 if pressed, 0 if not.
 */
uint8_t Touch_IsPressed(void);

/**
 * @brief  Reads and calculates the current touch coordinates in pixels.
 * @param  x: Pointer to store calculated X coordinate.
 * @param  y: Pointer to store calculated Y coordinate.
 * @return 1 if valid touch detected, 0 otherwise.
 */
uint8_t Touch_GetPixels(uint16_t *x, uint16_t *y);

/**
 * @brief  Checks if a specific touch coordinate falls within a button's boundaries.
 * @param  button: The button definition to check against.
 * @param  touch_x, touch_y: The active touch coordinates.
 * @return 1 if inside, 0 if outside.
 */
uint8_t Button_IsPressed(ButtonDef button, uint16_t touch_x, uint16_t touch_y);

/**
 * @brief  Generic button drawing function (basic rectangle with text).
 */
void Button_Draw(ButtonDef *btn, const char* label, uint16_t color, uint16_t text_color);

#endif // TOUCH_H