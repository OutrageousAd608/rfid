/**
  ******************************************************************************
  * @file    fonts.h
  * @brief   Header for Font Library.
  * Defines font structures and rendering functions.
  ******************************************************************************
  */

#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

// --- FONT STRUCTURE ---
typedef struct {
    uint8_t width;        // Character width in pixels
    uint8_t height;       // Character height in pixels
    const uint16_t *data; // Pointer to raw bitmap array
} FontDef;

// --- EXPORTED FONTS ---
extern FontDef Font_7x10;

// --- PROTOTYPES ---

/**
 * @brief  Draws a single character at the current cursor position.
 */
void LCD_WriteChar(char ch, FontDef font, uint16_t color, uint16_t bgcolor);

/**
 * @brief  Draws a null-terminated string at specific coordinates.
 */
void LCD_WriteString(const char* str, uint16_t x, uint16_t y, FontDef font, uint16_t color, uint16_t bgcolor);

#endif // FONTS_H