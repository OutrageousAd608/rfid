#include "touch.h"
#include "spi.h"
#include "ili9341.h"
#include "fonts.h"
#include <string.h>

// XPT2046 Commands
#define CMD_X_READ  0x90
#define CMD_Y_READ  0xD0

static void Sort_Array(uint16_t *arr, uint8_t n) {
    for (uint8_t i = 1; i < n; i++) {
        uint16_t key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
}

static uint16_t TP_ReadAxis_Raw(uint8_t cmd) {
    uint8_t data_tx[3] = {cmd, 0x00, 0x00};
    uint8_t data_rx[3];

    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi2, data_tx, data_rx, 3, 10);
    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET);

    return ((data_rx[1] << 8) | data_rx[2]) >> 3;
}

static uint16_t TP_ReadAxis(uint8_t cmd) {
    uint16_t samples[16];
    uint32_t total = 0;

    for (int i = 0; i < 16; i++) {
        samples[i] = TP_ReadAxis_Raw(cmd);
    }

    Sort_Array(samples, 16);

    for (int i = 4; i < 12; i++) {
        total += samples[i];
    }

    return total / 8;
}

uint8_t Touch_IsPressed(void) {
    return (HAL_GPIO_ReadPin(TOUCH_IRQ_GPIO_Port, TOUCH_IRQ_Pin) == GPIO_PIN_RESET);
}

uint8_t Touch_GetPixels(uint16_t *x, uint16_t *y) {

    if (!Touch_IsPressed()) return 0;

    HAL_Delay(20);

    if (!Touch_IsPressed()) return 0;

    uint16_t raw_x = TP_ReadAxis(CMD_X_READ);
    uint16_t raw_y = TP_ReadAxis(CMD_Y_READ);

    if (raw_x < 50 || raw_x > 4050) return 0;
    if (raw_y < 50 || raw_y > 4050) return 0;

    if (raw_x < RAW_X_MIN) raw_x = RAW_X_MIN;
    if (raw_x > RAW_X_MAX) raw_x = RAW_X_MAX;

    *y = (uint32_t)(raw_x - RAW_X_MIN) * ILI9341_HEIGHT / (RAW_X_MAX - RAW_X_MIN);

    if (raw_y < RAW_Y_MIN) raw_y = RAW_Y_MIN;
    if (raw_y > RAW_Y_MAX) raw_y = RAW_Y_MAX;

    *x = ILI9341_WIDTH - ((uint32_t)(raw_y - RAW_Y_MIN) * ILI9341_WIDTH / (RAW_Y_MAX - RAW_Y_MIN));

    return 1;
}

uint8_t Button_IsPressed(ButtonDef button, uint16_t touch_x, uint16_t touch_y) {

    if (touch_x >= button.x && touch_x <= (button.x + button.width) &&
        touch_y >= button.y && touch_y <= (button.y + button.height)) {
        return 1;
    }
    return 0;

}

void Button_Draw(ButtonDef *btn, const char* label, uint16_t color, uint16_t text_color) {
    
    LCD_FillRect(btn->x, btn->y, btn->width, btn->height, color);

    uint16_t text_len = strlen(label) * 7;
    uint16_t x_center;
    if (text_len < btn->width) {
        x_center = btn->x + (btn->width - text_len) / 2;
    } else {
        x_center = btn->x + 2;
    }
    uint16_t y_center = btn->y + (btn->height - 10) / 2;
    LCD_WriteString(label, x_center, y_center, Font_7x10, text_color, color);
    
}
