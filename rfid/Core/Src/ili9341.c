#include "ili9341.h"
#include "spi.h"

void LCD_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void LCD_WriteData(uint8_t data) {
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &data, 1, 10);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void LCD_WriteData16(uint16_t data) {
    uint8_t bytes[] = { (data >> 8), (data & 0xFF) };
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, bytes, 2, 10);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    LCD_WriteCommand(0x2A); // Column Address
    LCD_WriteData(x1 >> 8); LCD_WriteData(x1);
    LCD_WriteData(x2 >> 8); LCD_WriteData(x2);
    LCD_WriteCommand(0x2B); // Row Address
    LCD_WriteData(y1 >> 8); LCD_WriteData(y1);
    LCD_WriteData(y2 >> 8); LCD_WriteData(y2);
    LCD_WriteCommand(0x2C); // Memory Write
}

void LCD_Init(void) {
    // Hardware Reset
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    // Initialization Commands
    LCD_WriteCommand(0x01); HAL_Delay(100); // Sw Reset
    LCD_WriteCommand(0xCB); LCD_WriteData(0x39); LCD_WriteData(0x2C); LCD_WriteData(0x00); LCD_WriteData(0x34); LCD_WriteData(0x02);
    LCD_WriteCommand(0xCF); LCD_WriteData(0x00); LCD_WriteData(0xC1); LCD_WriteData(0x30);
    LCD_WriteCommand(0xE8); LCD_WriteData(0x85); LCD_WriteData(0x00); LCD_WriteData(0x78);
    LCD_WriteCommand(0xEA); LCD_WriteData(0x00); LCD_WriteData(0x00);
    LCD_WriteCommand(0xED); LCD_WriteData(0x64); LCD_WriteData(0x03); LCD_WriteData(0x12); LCD_WriteData(0x81);
    LCD_WriteCommand(0xF7); LCD_WriteData(0x20);
    LCD_WriteCommand(0xC0); LCD_WriteData(0x23);
    LCD_WriteCommand(0xC1); LCD_WriteData(0x10);
    LCD_WriteCommand(0xC5); LCD_WriteData(0x3E); LCD_WriteData(0x28);
    LCD_WriteCommand(0xC7); LCD_WriteData(0x86);

    // MADCTL: Orientation (0x48 = Portrait/Pins Down)
    LCD_WriteCommand(0x36); LCD_WriteData(0x48);

    LCD_WriteCommand(0x3A); LCD_WriteData(0x55); // Pixel Format 16bit
    LCD_WriteCommand(0xB1); LCD_WriteData(0x00); LCD_WriteData(0x18);
    LCD_WriteCommand(0xB6); LCD_WriteData(0x08); LCD_WriteData(0x82); LCD_WriteData(0x27);
    LCD_WriteCommand(0x11); HAL_Delay(120); // Sleep Out
    LCD_WriteCommand(0x29); // Display On
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if(x >= ILI9341_WIDTH || y >= ILI9341_HEIGHT) return;
    LCD_SetAddress(x, y, x, y);
    LCD_WriteData16(color);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if((x + w) > ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if((y + h) > ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    LCD_SetAddress(x, y, x+w-1, y+h-1);

    uint8_t bytes[] = { (color >> 8), (color & 0xFF) };
    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

    for(uint32_t i = 0; i < (w*h); i++) {
        HAL_SPI_Transmit(&hspi1, bytes, 2, 10);
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void LCD_FillColor(uint16_t color) {
    LCD_FillRect(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}
