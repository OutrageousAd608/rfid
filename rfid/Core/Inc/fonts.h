#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;

extern FontDef Font_7x10;

void LCD_WriteChar(char ch, FontDef font, uint16_t color, uint16_t bgcolor);
void LCD_WriteString(const char* str, uint16_t x, uint16_t y, FontDef font, uint16_t color, uint16_t bgcolor);

#endif
