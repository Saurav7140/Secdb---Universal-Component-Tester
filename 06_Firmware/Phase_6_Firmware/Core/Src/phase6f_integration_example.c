/* CubeMX integration skeleton; adapt the two port callbacks to the project. */
#include "main.h"
#include "secdb_adc.h"
#include "secdb_afe.h"
#include "secdb_app.h"
#include "secdb_calibration_store.h"
#include "secdb_log.h"
#include "secdb_st7735.h"
#include "secdb_timebase.h"

extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi1;

/* Return false until a reserved flash page or external EEPROM is assigned. */
static bool calibration_read_port(void *data, size_t size)
{
    (void)data;
    (void)size;
    return false;
}

static bool calibration_write_port(const void *data, size_t size)
{
    (void)data;
    (void)size;
    return false;
}

/* Replace with CDC_Transmit_FS or a UART transmit wrapper. */
static bool diagnostic_write_port(const uint8_t *data, uint16_t length)
{
    (void)data;
    (void)length;
    return false;
}

void SECDB_Phase6F_RunExample(void)
{
    static secdb_app_t app;

    if (!SECDB_Timebase_Init()) Error_Handler();
    SECDB_AFE_Init();
    if (SECDB_ADC_Bind(&hadc1) != SECDB_ADC_OK) Error_Handler();

    SECDB_Calibration_BindStorage(
        calibration_read_port, calibration_write_port);
    SECDB_Log_Bind(diagnostic_write_port);

    if (SECDB_ST7735_Init(&hspi1) != SECDB_ST7735_OK) Error_Handler();
    if (SECDB_App_Init(&app) != SECDB_APP_OK) Error_Handler();

    for (;;) {
        SECDB_App_Tick(&app);
    }
}
