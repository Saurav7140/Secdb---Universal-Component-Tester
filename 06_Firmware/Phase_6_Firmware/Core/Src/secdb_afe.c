#include "secdb_afe.h"
#include "secdb_board.h"
#include "secdb_timebase.h"
#include <stddef.h>

#define SECDB_BREAK_BEFORE_MAKE_US  2UL
#define SECDB_ANALOG_SETTLE_US      200UL

typedef enum {
    SRC_1K = 0,
    SRC_10K = 1,
    SRC_100K = 2,
    SRC_DISCHARGE = 3
} source_channel_t;

typedef enum {
    SNS_RC = 0,
    SNS_DIODE_A = 1,
    SNS_DIODE_B = 2,
    SNS_BJT_E = 3,
    SNS_BJT_B = 4,
    SNS_BJT_C = 5,
    SNS_CAL_1V1 = 6,
    SNS_CAL_2V2 = 7
} sense_channel_t;

static secdb_afe_mode_t current_mode = SECDB_AFE_DISABLED;
static bool measurement_permitted;

static bool mode_requires_calibration(secdb_afe_mode_t mode)
{
    return (mode != SECDB_AFE_DISABLED) &&
           (mode != SECDB_AFE_CAL_1V1) &&
           (mode != SECDB_AFE_CAL_2V2);
}

static void output_pin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    GPIO_InitTypeDef gpio = {0};

    HAL_GPIO_WritePin(port, pin, state);
    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);
}

static void analog_pin(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(port, &gpio);
}

static void set_source_address(source_channel_t channel)
{
    HAL_GPIO_WritePin(SECDB_SRC_A0_GPIO_Port, SECDB_SRC_A0_Pin,
                      ((uint32_t)channel & 0x01UL) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECDB_SRC_A1_GPIO_Port, SECDB_SRC_A1_Pin,
                      ((uint32_t)channel & 0x02UL) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void set_sense_address(sense_channel_t channel)
{
    HAL_GPIO_WritePin(SECDB_SNS_A0_GPIO_Port, SECDB_SNS_A0_Pin,
                      ((uint32_t)channel & 0x01UL) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECDB_SNS_A1_GPIO_Port, SECDB_SNS_A1_Pin,
                      ((uint32_t)channel & 0x02UL) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECDB_SNS_A2_GPIO_Port, SECDB_SNS_A2_Pin,
                      ((uint32_t)channel & 0x04UL) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void enable_source(source_channel_t channel)
{
    set_source_address(channel);
    HAL_GPIO_WritePin(SECDB_SRC_EN_GPIO_Port, SECDB_SRC_EN_Pin, GPIO_PIN_SET);
}

static void enable_sense(sense_channel_t channel)
{
    set_sense_address(channel);
    HAL_GPIO_WritePin(SECDB_SNS_EN_GPIO_Port, SECDB_SNS_EN_Pin, GPIO_PIN_SET);
}

static void drive(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    output_pin(port, pin, state);
}

static void tri_state_all_drives(void)
{
    /* BJT base drive is removed first by design. */
    analog_pin(SECDB_BJT_B_GPIO_Port, SECDB_BJT_B_Pin);
    analog_pin(SECDB_BJT_E_GPIO_Port, SECDB_BJT_E_Pin);
    analog_pin(SECDB_BJT_C_GPIO_Port, SECDB_BJT_C_Pin);
    analog_pin(SECDB_DIO_A_GPIO_Port, SECDB_DIO_A_Pin);
    analog_pin(SECDB_DIO_B_GPIO_Port, SECDB_DIO_B_Pin);
}

static void apply_diode_drive(GPIO_PinState a, GPIO_PinState b)
{
    /* Configure the low leg before the high leg for either polarity. */
    if ((a == GPIO_PIN_SET) && (b == GPIO_PIN_RESET)) {
        drive(SECDB_DIO_B_GPIO_Port, SECDB_DIO_B_Pin, b);
        drive(SECDB_DIO_A_GPIO_Port, SECDB_DIO_A_Pin, a);
    } else {
        drive(SECDB_DIO_A_GPIO_Port, SECDB_DIO_A_Pin, a);
        drive(SECDB_DIO_B_GPIO_Port, SECDB_DIO_B_Pin, b);
    }
}

static void apply_bjt_drive(GPIO_PinState emitter,
                            GPIO_PinState base,
                            GPIO_PinState collector)
{
    /* The mandatory order is emitter, collector, then base. */
    drive(SECDB_BJT_E_GPIO_Port, SECDB_BJT_E_Pin, emitter);
    drive(SECDB_BJT_C_GPIO_Port, SECDB_BJT_C_Pin, collector);
    drive(SECDB_BJT_B_GPIO_Port, SECDB_BJT_B_Pin, base);
}

static void finish_sense_selection(sense_channel_t channel)
{
    enable_sense(channel);
    SECDB_DelayUs(SECDB_ANALOG_SETTLE_US);
}

void SECDB_AFE_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Write and configure both enables low before any address/output setup. */
    output_pin(SECDB_SNS_EN_GPIO_Port, SECDB_SNS_EN_Pin, GPIO_PIN_RESET);
    output_pin(SECDB_SRC_EN_GPIO_Port, SECDB_SRC_EN_Pin, GPIO_PIN_RESET);

    output_pin(SECDB_SRC_A0_GPIO_Port, SECDB_SRC_A0_Pin, GPIO_PIN_RESET);
    output_pin(SECDB_SRC_A1_GPIO_Port, SECDB_SRC_A1_Pin, GPIO_PIN_RESET);
    output_pin(SECDB_SNS_A0_GPIO_Port, SECDB_SNS_A0_Pin, GPIO_PIN_RESET);
    output_pin(SECDB_SNS_A1_GPIO_Port, SECDB_SNS_A1_Pin, GPIO_PIN_RESET);
    output_pin(SECDB_SNS_A2_GPIO_Port, SECDB_SNS_A2_Pin, GPIO_PIN_RESET);

    tri_state_all_drives();
    analog_pin(SECDB_ADC_GPIO_Port, SECDB_ADC_Pin);
    measurement_permitted = false;
    current_mode = SECDB_AFE_DISABLED;
}

void SECDB_AFE_EnterSafeTransition(void)
{
    HAL_GPIO_WritePin(SECDB_SNS_EN_GPIO_Port, SECDB_SNS_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SECDB_SRC_EN_GPIO_Port, SECDB_SRC_EN_Pin, GPIO_PIN_RESET);
    SECDB_DelayUs(SECDB_BREAK_BEFORE_MAKE_US);

    tri_state_all_drives();
    set_source_address(SRC_1K);
    set_sense_address(SNS_RC);
    current_mode = SECDB_AFE_DISABLED;
}

void SECDB_AFE_SetMeasurementPermit(bool permitted)
{
    measurement_permitted = permitted;
    if (!permitted) {
        SECDB_AFE_EnterSafeTransition();
    }
}

bool SECDB_AFE_IsMeasurementPermitted(void)
{
    return measurement_permitted;
}

secdb_status_t SECDB_AFE_ApplyMode(secdb_afe_mode_t mode)
{
    SECDB_AFE_EnterSafeTransition();

    if ((uint32_t)mode > (uint32_t)SECDB_AFE_PNP_BASE_OFF_READ_C) {
        return SECDB_STATUS_INVALID_ARGUMENT;
    }
    if (mode_requires_calibration(mode) && !measurement_permitted) {
        return SECDB_STATUS_CALIBRATION_REQUIRED;
    }

    switch (mode) {
    case SECDB_AFE_DISABLED:
        return SECDB_STATUS_OK;

    case SECDB_AFE_CAL_1V1:
        finish_sense_selection(SNS_CAL_1V1);
        break;
    case SECDB_AFE_CAL_2V2:
        finish_sense_selection(SNS_CAL_2V2);
        break;

    case SECDB_AFE_RESISTOR_10K_TRIAL:
        enable_source(SRC_10K);
        finish_sense_selection(SNS_RC);
        break;
    case SECDB_AFE_RESISTOR_1K:
        enable_source(SRC_1K);
        finish_sense_selection(SNS_RC);
        break;
    case SECDB_AFE_RESISTOR_100K:
        enable_source(SRC_100K);
        finish_sense_selection(SNS_RC);
        break;

    case SECDB_AFE_CAP_DISCHARGE:
        enable_source(SRC_DISCHARGE);
        finish_sense_selection(SNS_RC);
        break;
    case SECDB_AFE_CAP_READY:
        finish_sense_selection(SNS_RC);
        break;
    case SECDB_AFE_CAP_CHARGE:
        return SECDB_STATUS_BAD_SEQUENCE;

    case SECDB_AFE_DIODE_AB_READ_A:
        apply_diode_drive(GPIO_PIN_SET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_DIODE_A);
        break;
    case SECDB_AFE_DIODE_AB_READ_B:
        apply_diode_drive(GPIO_PIN_SET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_DIODE_B);
        break;
    case SECDB_AFE_DIODE_BA_READ_A:
        apply_diode_drive(GPIO_PIN_RESET, GPIO_PIN_SET);
        finish_sense_selection(SNS_DIODE_A);
        break;
    case SECDB_AFE_DIODE_BA_READ_B:
        apply_diode_drive(GPIO_PIN_RESET, GPIO_PIN_SET);
        finish_sense_selection(SNS_DIODE_B);
        break;

    case SECDB_AFE_NPN_READ_E:
        apply_bjt_drive(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET);
        finish_sense_selection(SNS_BJT_E);
        break;
    case SECDB_AFE_NPN_READ_B:
        apply_bjt_drive(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET);
        finish_sense_selection(SNS_BJT_B);
        break;
    case SECDB_AFE_NPN_READ_C:
        apply_bjt_drive(GPIO_PIN_RESET, GPIO_PIN_SET, GPIO_PIN_SET);
        finish_sense_selection(SNS_BJT_C);
        break;
    case SECDB_AFE_NPN_BASE_OFF_READ_E:
        apply_bjt_drive(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET);
        finish_sense_selection(SNS_BJT_E);
        break;
    case SECDB_AFE_NPN_BASE_OFF_READ_C:
        apply_bjt_drive(GPIO_PIN_RESET, GPIO_PIN_RESET, GPIO_PIN_SET);
        finish_sense_selection(SNS_BJT_C);
        break;

    case SECDB_AFE_PNP_READ_E:
        apply_bjt_drive(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_BJT_E);
        break;
    case SECDB_AFE_PNP_READ_B:
        apply_bjt_drive(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_BJT_B);
        break;
    case SECDB_AFE_PNP_READ_C:
        apply_bjt_drive(GPIO_PIN_SET, GPIO_PIN_RESET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_BJT_C);
        break;
    case SECDB_AFE_PNP_BASE_OFF_READ_E:
        apply_bjt_drive(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_BJT_E);
        break;
    case SECDB_AFE_PNP_BASE_OFF_READ_C:
        apply_bjt_drive(GPIO_PIN_SET, GPIO_PIN_SET, GPIO_PIN_RESET);
        finish_sense_selection(SNS_BJT_C);
        break;

    default:
        return SECDB_STATUS_INVALID_ARGUMENT;
    }

    current_mode = mode;
    return SECDB_STATUS_OK;
}

secdb_status_t SECDB_AFE_StartCapCharge(uint32_t *start_time_us)
{
    if ((current_mode != SECDB_AFE_CAP_READY) || (start_time_us == NULL)) {
        return SECDB_STATUS_BAD_SEQUENCE;
    }

    set_source_address(SRC_10K);
    HAL_GPIO_WritePin(SECDB_SRC_EN_GPIO_Port, SECDB_SRC_EN_Pin, GPIO_PIN_SET);
    *start_time_us = SECDB_TimeUs();
    current_mode = SECDB_AFE_CAP_CHARGE;

    return SECDB_STATUS_OK;
}

secdb_afe_mode_t SECDB_AFE_GetMode(void)
{
    return current_mode;
}
