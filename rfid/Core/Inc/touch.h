#ifndef TOUCH_H
#define TOUCH_H

#include "main.h"

#define RAW_X_MIN  200
#define RAW_X_MAX  3700
#define RAW_Y_MIN  300
#define RAW_Y_MAX  3800

#define ILI9341_WIDTH  320
#define ILI9341_HEIGHT 240

typedef struct {
    uint16_t x;       // Top-left X position
    uint16_t y;       // Top-left Y position
    uint16_t width;   // Width of the button
    uint16_t height;  // Height of the button
} ButtonDef;

uint8_t Touch_IsPressed(void);
uint8_t Touch_GetPixels(uint16_t *x, uint16_t *y);

uint8_t Button_IsPressed(ButtonDef button, uint16_t touch_x, uint16_t touch_y);
void Button_Draw(ButtonDef *btn, const char* label, uint16_t color, uint16_t text_color);

#endif
