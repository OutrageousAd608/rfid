/**
  ******************************************************************************
  * @file    ili9341.h
  * @brief   Header for ILI9341 TFT LCD Driver.
  * Defines colors, screen resolution, and hardware interface functions.
  ******************************************************************************
  */

#ifndef ILI9341_H
#define ILI9341_H

#include "main.h"

// --- COLOR DEFINITIONS (RGB565 Format) ---
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// --- SCREEN DIMENSIONS ---
// Orientation is configured in LCD_Init (0x36 command)
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320

// --- FUNCTION PROTOTYPES ---

/**
 * @brief  Initialize the display hardware, SPI interface, and default orientation.
 * @note   Must be called before any drawing functions.
 */
void LCD_Init(void);

/**
 * @brief  Fills the entire screen with a specific color.
 * @param  color: 16-bit RGB565 color value.
 */
void LCD_FillColor(uint16_t color);

/**
 * @brief  Draws a single pixel at the specified coordinates.
 * @param  x: X-coordinate.
 * @param  y: Y-coordinate.
 * @param  color: 16-bit RGB565 color.
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief  Draws a filled rectangle.
 * @param  x, y: Top-left corner coordinates.
 * @param  w, h: Width and Height of the rectangle.
 * @param  color: Fill color.
 */
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief  Sets the active drawing window (Address Window).
 * @note   Used internally by drawing functions to define where data is written.
 */
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief  Sends a 16-bit data word to the display (split into two 8-bit SPI transfers).
 * @param  data: 16-bit data to send.
 */
void LCD_WriteData16(uint16_t data);

#endif // ILI9341_H