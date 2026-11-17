#include "secdb_st7735.h"
#include "secdb_board.h"
#include <stddef.h>

#define ST7735_SWRESET  0x01U
#define ST7735_SLPOUT   0x11U
#define ST7735_NORON    0x13U
#define ST7735_INVOFF   0x20U
#define ST7735_DISPON   0x29U
#define ST7735_CASET    0x2AU
#define ST7735_RASET    0x2BU
#define ST7735_RAMWR    0x2CU
#define ST7735_MADCTL   0x36U
#define ST7735_COLMOD   0x3AU
#define ST7735_FRMCTR1  0xB1U
#define ST7735_FRMCTR2  0xB2U
#define ST7735_FRMCTR3  0xB3U
#define ST7735_INVCTR   0xB4U
#define ST7735_PWCTR1   0xC0U
#define ST7735_PWCTR2   0xC1U
#define ST7735_PWCTR3   0xC2U
#define ST7735_PWCTR4   0xC3U
#define ST7735_PWCTR5   0xC4U
#define ST7735_VMCTR1   0xC5U
#define ST7735_SPI_TIMEOUT_MS  50UL

static SPI_HandleTypeDef *display_spi;
static bool display_ready;

static const uint8_t digit_font[10][5] = {
    {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
    {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
    {0x42U, 0x61U, 0x51U, 0x49U, 0x46U},
    {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U},
    {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
    {0x27U, 0x45U, 0x45U, 0x45U, 0x39U},
    {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U},
    {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
    {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
    {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU}
};

static const uint8_t upper_font[26][5] = {
    {0x7EU,0x11U,0x11U,0x11U,0x7EU}, {0x7FU,0x49U,0x49U,0x49U,0x36U},
    {0x3EU,0x41U,0x41U,0x41U,0x22U}, {0x7FU,0x41U,0x41U,0x22U,0x1CU},
    {0x7FU,0x49U,0x49U,0x49U,0x41U}, {0x7FU,0x09U,0x09U,0x09U,0x01U},
    {0x3EU,0x41U,0x49U,0x49U,0x7AU}, {0x7FU,0x08U,0x08U,0x08U,0x7FU},
    {0x00U,0x41U,0x7FU,0x41U,0x00U}, {0x20U,0x40U,0x41U,0x3FU,0x01U},
    {0x7FU,0x08U,0x14U,0x22U,0x41U}, {0x7FU,0x40U,0x40U,0x40U,0x40U},
    {0x7FU,0x02U,0x0CU,0x02U,0x7FU}, {0x7FU,0x04U,0x08U,0x10U,0x7FU},
    {0x3EU,0x41U,0x41U,0x41U,0x3EU}, {0x7FU,0x09U,0x09U,0x09U,0x06U},
    {0x3EU,0x41U,0x51U,0x21U,0x5EU}, {0x7FU,0x09U,0x19U,0x29U,0x46U},
    {0x46U,0x49U,0x49U,0x49U,0x31U}, {0x01U,0x01U,0x7FU,0x01U,0x01U},
    {0x3FU,0x40U,0x40U,0x40U,0x3FU}, {0x1FU,0x20U,0x40U,0x20U,0x1FU},
    {0x3FU,0x40U,0x38U,0x40U,0x3FU}, {0x63U,0x14U,0x08U,0x14U,0x63U},
    {0x07U,0x08U,0x70U,0x08U,0x07U}, {0x61U,0x51U,0x49U,0x45U,0x43U}
};

static bool transmit(const uint8_t *data, uint16_t length)
{
    return HAL_SPI_Transmit(display_spi, (uint8_t *)data, length,
                            ST7735_SPI_TIMEOUT_MS) == HAL_OK;
}

static bool command_data(uint8_t command, const uint8_t *data, uint16_t length)
{
    bool ok;
    HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECDB_TFT_DC_GPIO_Port, SECDB_TFT_DC_Pin, GPIO_PIN_RESET);
    ok = transmit(&command, 1U);
    if (ok && (length != 0U)) {
        HAL_GPIO_WritePin(SECDB_TFT_DC_GPIO_Port, SECDB_TFT_DC_Pin,
                          GPIO_PIN_SET);
        ok = transmit(data, length);
    }
    HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin, GPIO_PIN_SET);
    return ok;
}

static bool address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];
    data[0] = (uint8_t)(x0 >> 8U); data[1] = (uint8_t)x0;
    data[2] = (uint8_t)(x1 >> 8U); data[3] = (uint8_t)x1;
    if (!command_data(ST7735_CASET, data, 4U)) {
        return false;
    }
    data[0] = (uint8_t)(y0 >> 8U); data[1] = (uint8_t)y0;
    data[2] = (uint8_t)(y1 >> 8U); data[3] = (uint8_t)y1;
    return command_data(ST7735_RASET, data, 4U);
}

static const uint8_t *glyph_for(char character, uint8_t glyph[5])
{
    uint8_t index;
    if ((character >= 'a') && (character <= 'z')) {
        character = (char)(character - 'a' + 'A');
    }
    if ((character >= '0') && (character <= '9')) {
        return digit_font[(uint8_t)(character - '0')];
    }
    if ((character >= 'A') && (character <= 'Z')) {
        return upper_font[(uint8_t)(character - 'A')];
    }
    for (index = 0U; index < 5U; ++index) {
        glyph[index] = 0U;
    }
    switch (character) {
    case '-': glyph[0]=0x08U; glyph[1]=0x08U; glyph[2]=0x08U;
              glyph[3]=0x08U; glyph[4]=0x08U; break;
    case '_': glyph[0]=0x40U; glyph[1]=0x40U; glyph[2]=0x40U;
              glyph[3]=0x40U; glyph[4]=0x40U; break;
    case '.': glyph[2]=0x60U; glyph[3]=0x60U; break;
    case ':': glyph[2]=0x36U; glyph[3]=0x36U; break;
    case '/': glyph[0]=0x20U; glyph[1]=0x10U; glyph[2]=0x08U;
              glyph[3]=0x04U; glyph[4]=0x02U; break;
    case '+': glyph[0]=0x08U; glyph[1]=0x08U; glyph[2]=0x3EU;
              glyph[3]=0x08U; glyph[4]=0x08U; break;
    case '%': glyph[0]=0x62U; glyph[1]=0x64U; glyph[2]=0x08U;
              glyph[3]=0x13U; glyph[4]=0x23U; break;
    case '=': glyph[0]=0x14U; glyph[1]=0x14U; glyph[2]=0x14U;
              glyph[3]=0x14U; glyph[4]=0x14U; break;
    case '?': glyph[0]=0x02U; glyph[1]=0x01U; glyph[2]=0x51U;
              glyph[3]=0x09U; glyph[4]=0x06U; break;
    default: break;
    }
    return glyph;
}

static secdb_st7735_status_t draw_character(
    uint16_t x, uint16_t y, char character, uint8_t scale,
    uint16_t foreground, uint16_t background)
{
    uint8_t pixels[384];
    uint8_t local_glyph[5];
    const uint8_t *glyph = glyph_for(character, local_glyph);
    uint16_t output_width = (uint16_t)(6U * scale);
    uint16_t output_height = (uint16_t)(8U * scale);
    uint16_t output_index = 0U;
    uint16_t row;
    uint16_t column;
    uint8_t repeat_x;
    uint8_t repeat_y;
    bool foreground_pixel;
    uint16_t color;

    if ((scale == 0U) || (scale > 2U) ||
        ((uint32_t)x + output_width > SECDB_ST7735_WIDTH) ||
        ((uint32_t)y + output_height > SECDB_ST7735_HEIGHT)) {
        return SECDB_ST7735_INVALID_ARGUMENT;
    }
    for (row = 0U; row < 8U; ++row) {
        for (repeat_y = 0U; repeat_y < scale; ++repeat_y) {
            for (column = 0U; column < 6U; ++column) {
                foreground_pixel = (column < 5U) && (row < 7U) &&
                    ((glyph[column] & (uint8_t)(1U << row)) != 0U);
                color = foreground_pixel ? foreground : background;
                for (repeat_x = 0U; repeat_x < scale; ++repeat_x) {
                    pixels[output_index++] = (uint8_t)(color >> 8U);
                    pixels[output_index++] = (uint8_t)color;
                }
            }
        }
    }
    if (!address_window(x, y, (uint16_t)(x + output_width - 1U),
                        (uint16_t)(y + output_height - 1U))) {
        return SECDB_ST7735_SPI_FAILURE;
    }
    if (!command_data(ST7735_RAMWR, pixels, output_index)) {
        return SECDB_ST7735_SPI_FAILURE;
    }
    return SECDB_ST7735_OK;
}

secdb_st7735_status_t SECDB_ST7735_Init(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio = {0};
    const uint8_t frame[3] = {0x01U, 0x2CU, 0x2DU};
    const uint8_t frame3[6] = {0x01U,0x2CU,0x2DU,0x01U,0x2CU,0x2DU};
    const uint8_t pw1[3] = {0xA2U, 0x02U, 0x84U};
    const uint8_t pw3[2] = {0x0AU, 0x00U};
    const uint8_t pw4[2] = {0x8AU, 0x2AU};
    const uint8_t pw5[2] = {0x8AU, 0xEEU};
    const uint8_t one_c5 = 0xC5U;
    const uint8_t one_07 = 0x07U;
    const uint8_t one_0e = 0x0EU;
    const uint8_t one_c8 = 0xC8U;
    const uint8_t one_05 = 0x05U;

    display_ready = false;
    if ((hspi == NULL) || (hspi->Instance != SPI1)) {
        return SECDB_ST7735_INVALID_ARGUMENT;
    }
    display_spi = hspi;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = SECDB_TFT_CS_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SECDB_TFT_CS_GPIO_Port, &gpio);
    gpio.Pin = SECDB_TFT_RST_Pin | SECDB_TFT_DC_Pin;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SECDB_TFT_RST_GPIO_Port, SECDB_TFT_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(20UL);
    HAL_GPIO_WritePin(SECDB_TFT_RST_GPIO_Port, SECDB_TFT_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120UL);

    if (!command_data(ST7735_SWRESET, NULL, 0U)) return SECDB_ST7735_SPI_FAILURE;
    HAL_Delay(150UL);
    if (!command_data(ST7735_SLPOUT, NULL, 0U)) return SECDB_ST7735_SPI_FAILURE;
    HAL_Delay(120UL);
    if (!command_data(ST7735_FRMCTR1, frame, 3U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_FRMCTR2, frame, 3U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_FRMCTR3, frame3, 6U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_INVCTR, &one_07, 1U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_PWCTR1, pw1, 3U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_PWCTR2, &one_c5, 1U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_PWCTR3, pw3, 2U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_PWCTR4, pw4, 2U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_PWCTR5, pw5, 2U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_VMCTR1, &one_0e, 1U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_INVOFF, NULL, 0U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_MADCTL, &one_c8, 1U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_COLMOD, &one_05, 1U)) return SECDB_ST7735_SPI_FAILURE;
    if (!command_data(ST7735_NORON, NULL, 0U)) return SECDB_ST7735_SPI_FAILURE;
    HAL_Delay(10UL);
    if (!command_data(ST7735_DISPON, NULL, 0U)) return SECDB_ST7735_SPI_FAILURE;
    HAL_Delay(100UL);
    display_ready = true;
    return SECDB_ST7735_Clear(SECDB_COLOR_BLACK);
}

secdb_st7735_status_t SECDB_ST7735_FillRectangle(
    uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint8_t pixels[128];
    uint32_t remaining;
    uint16_t chunk;
    uint16_t index;

    if (!display_ready) return SECDB_ST7735_NOT_INITIALIZED;
    if ((width == 0U) || (height == 0U) ||
        ((uint32_t)x + width > SECDB_ST7735_WIDTH) ||
        ((uint32_t)y + height > SECDB_ST7735_HEIGHT)) {
        return SECDB_ST7735_INVALID_ARGUMENT;
    }
    if (!address_window(x, y, (uint16_t)(x + width - 1U),
                        (uint16_t)(y + height - 1U))) {
        return SECDB_ST7735_SPI_FAILURE;
    }
    for (index = 0U; index < sizeof(pixels); index += 2U) {
        pixels[index] = (uint8_t)(color >> 8U);
        pixels[index + 1U] = (uint8_t)color;
    }
    remaining = (uint32_t)width * height * 2UL;
    HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECDB_TFT_DC_GPIO_Port, SECDB_TFT_DC_Pin, GPIO_PIN_RESET);
    {
        const uint8_t command = ST7735_RAMWR;
        if (!transmit(&command, 1U)) {
            HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin,
                              GPIO_PIN_SET);
            return SECDB_ST7735_SPI_FAILURE;
        }
    }
    HAL_GPIO_WritePin(SECDB_TFT_DC_GPIO_Port, SECDB_TFT_DC_Pin, GPIO_PIN_SET);
    while (remaining != 0UL) {
        chunk = (remaining > sizeof(pixels)) ? (uint16_t)sizeof(pixels)
                                            : (uint16_t)remaining;
        if (!transmit(pixels, chunk)) {
            HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin,
                              GPIO_PIN_SET);
            return SECDB_ST7735_SPI_FAILURE;
        }
        remaining -= chunk;
    }
    HAL_GPIO_WritePin(SECDB_TFT_CS_GPIO_Port, SECDB_TFT_CS_Pin, GPIO_PIN_SET);
    return SECDB_ST7735_OK;
}

secdb_st7735_status_t SECDB_ST7735_Clear(uint16_t color)
{
    return SECDB_ST7735_FillRectangle(
        0U, 0U, SECDB_ST7735_WIDTH, SECDB_ST7735_HEIGHT, color);
}

secdb_st7735_status_t SECDB_ST7735_WriteText(
    uint16_t x, uint16_t y, const char *text, uint8_t scale,
    uint16_t foreground, uint16_t background)
{
    secdb_st7735_status_t status;
    uint16_t cursor_x = x;
    const uint16_t advance = (uint16_t)(6U * scale);
    if (!display_ready) return SECDB_ST7735_NOT_INITIALIZED;
    if ((text == NULL) || (scale == 0U) || (scale > 2U)) {
        return SECDB_ST7735_INVALID_ARGUMENT;
    }
    while (*text != '\0') {
        if ((uint32_t)cursor_x + advance > SECDB_ST7735_WIDTH) {
            break;
        }
        status = draw_character(cursor_x, y, *text, scale,
                                foreground, background);
        if (status != SECDB_ST7735_OK) return status;
        cursor_x = (uint16_t)(cursor_x + advance);
        ++text;
    }
    return SECDB_ST7735_OK;
}

bool SECDB_ST7735_IsReady(void)
{
    return display_ready;
}
