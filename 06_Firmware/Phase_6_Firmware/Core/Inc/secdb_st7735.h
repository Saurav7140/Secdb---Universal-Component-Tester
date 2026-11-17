#ifndef SECDB_ST7735_H
#define SECDB_ST7735_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define SECDB_ST7735_WIDTH   128U
#define SECDB_ST7735_HEIGHT  160U

#define SECDB_COLOR_BLACK    0x0000U
#define SECDB_COLOR_WHITE    0xFFFFU
#define SECDB_COLOR_RED      0xF800U
#define SECDB_COLOR_GREEN    0x07E0U
#define SECDB_COLOR_BLUE     0x001FU
#define SECDB_COLOR_YELLOW   0xFFE0U
#define SECDB_COLOR_CYAN     0x07FFU
#define SECDB_COLOR_ORANGE   0xFD20U
#define SECDB_COLOR_GRAY     0x8410U

typedef enum {
    SECDB_ST7735_OK = 0,
    SECDB_ST7735_INVALID_ARGUMENT,
    SECDB_ST7735_NOT_INITIALIZED,
    SECDB_ST7735_SPI_FAILURE
} secdb_st7735_status_t;

secdb_st7735_status_t SECDB_ST7735_Init(SPI_HandleTypeDef *hspi);
secdb_st7735_status_t SECDB_ST7735_Clear(uint16_t color);
secdb_st7735_status_t SECDB_ST7735_FillRectangle(
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
secdb_st7735_status_t SECDB_ST7735_WriteText(
    uint16_t x, uint16_t y, const char *text, uint8_t scale,
    uint16_t foreground, uint16_t background);
bool SECDB_ST7735_IsReady(void);

#endif /* SECDB_ST7735_H */
