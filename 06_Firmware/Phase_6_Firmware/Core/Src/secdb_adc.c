#include "secdb_adc.h"
#include "secdb_afe.h"
#include "secdb_board.h"
#include <stddef.h>

static ADC_HandleTypeDef *adc_handle;
static secdb_adc_calibration_t calibration;

static secdb_adc_status_t fail_safe(secdb_adc_status_t status)
{
    SECDB_AFE_EnterSafeTransition();
    return status;
}

static bool in_window(uint16_t value, uint16_t target, uint16_t tolerance)
{
    const uint16_t lower = (target > tolerance) ? (uint16_t)(target - tolerance) : 0U;
    const uint16_t upper = (uint16_t)(target + tolerance);
    return (value >= lower) && (value <= upper);
}

static secdb_adc_status_t convert_once(uint32_t timeout_ms, uint16_t *code)
{
    HAL_StatusTypeDef hal_status;
    uint32_t value;

    hal_status = HAL_ADC_Start(adc_handle);
    if (hal_status != HAL_OK) {
        return SECDB_ADC_HAL_ERROR;
    }

    hal_status = HAL_ADC_PollForConversion(adc_handle, timeout_ms);
    if (hal_status != HAL_OK) {
        (void)HAL_ADC_Stop(adc_handle);
        return (hal_status == HAL_TIMEOUT) ? SECDB_ADC_TIMEOUT : SECDB_ADC_HAL_ERROR;
    }

    value = HAL_ADC_GetValue(adc_handle);
    hal_status = HAL_ADC_Stop(adc_handle);
    if (hal_status != HAL_OK) {
        return SECDB_ADC_HAL_ERROR;
    }

    if (value > SECDB_ADC_FULL_SCALE_CODE) {
        value = SECDB_ADC_FULL_SCALE_CODE;
    }
    *code = (uint16_t)value;
    return SECDB_ADC_OK;
}

secdb_adc_status_t SECDB_ADC_Bind(ADC_HandleTypeDef *hadc)
{
    ADC_ChannelConfTypeDef channel = {0};

    SECDB_ADC_InvalidateCalibration();
    adc_handle = NULL;

    if ((hadc == NULL) || (hadc->Instance != ADC1)) {
        return SECDB_ADC_INVALID_ARGUMENT;
    }
    if ((hadc->Init.Resolution != ADC_RESOLUTION_12B) ||
        (hadc->Init.ScanConvMode != DISABLE) ||
        (hadc->Init.ContinuousConvMode != DISABLE) ||
        (hadc->Init.DataAlign != ADC_DATAALIGN_RIGHT) ||
        (hadc->Init.NbrOfConversion != 1U)) {
        return SECDB_ADC_INVALID_ARGUMENT;
    }

    adc_handle = hadc;
    channel.Channel = SECDB_ADC_Channel;
    channel.Rank = 1U;
    channel.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    if (HAL_ADC_ConfigChannel(adc_handle, &channel) != HAL_OK) {
        adc_handle = NULL;
        return SECDB_ADC_HAL_ERROR;
    }
    return SECDB_ADC_OK;
}

secdb_adc_status_t SECDB_ADC_ReadRaw(uint32_t timeout_ms, uint16_t *code)
{
    secdb_adc_status_t status;

    if (adc_handle == NULL) {
        return SECDB_ADC_NOT_INITIALIZED;
    }
    if ((code == NULL) || (timeout_ms == 0UL)) {
        return fail_safe(SECDB_ADC_INVALID_ARGUMENT);
    }
    if (SECDB_AFE_GetMode() == SECDB_AFE_DISABLED) {
        return SECDB_ADC_AFE_NOT_CONNECTED;
    }

    status = convert_once(timeout_ms, code);
    return (status == SECDB_ADC_OK) ? status : fail_safe(status);
}

secdb_adc_status_t SECDB_ADC_ReadAveraged(uint16_t sample_count,
                                          uint32_t timeout_ms,
                                          secdb_adc_reading_t *reading)
{
    secdb_adc_status_t status;
    uint16_t discarded;
    uint16_t sample;
    uint16_t minimum = SECDB_ADC_FULL_SCALE_CODE;
    uint16_t maximum = 0U;
    uint32_t sum = 0UL;
    uint16_t index;
    uint16_t retained;

    if (adc_handle == NULL) {
        return SECDB_ADC_NOT_INITIALIZED;
    }
    if ((reading == NULL) || (timeout_ms == 0UL) ||
        (sample_count < SECDB_ADC_MIN_SAMPLE_COUNT) ||
        (sample_count > SECDB_ADC_MAX_SAMPLE_COUNT)) {
        return fail_safe(SECDB_ADC_INVALID_ARGUMENT);
    }
    if (SECDB_AFE_GetMode() == SECDB_AFE_DISABLED) {
        return SECDB_ADC_AFE_NOT_CONNECTED;
    }

    /* Mandatory throwaway conversion after every ordinary sense selection. */
    status = convert_once(timeout_ms, &discarded);
    if (status != SECDB_ADC_OK) {
        return fail_safe(status);
    }

    for (index = 0U; index < sample_count; ++index) {
        status = convert_once(timeout_ms, &sample);
        if (status != SECDB_ADC_OK) {
            return fail_safe(status);
        }
        sum += sample;
        if (sample < minimum) {
            minimum = sample;
        }
        if (sample > maximum) {
            maximum = sample;
        }
    }

    /* Remove one minimum and one maximum sample before rounding the mean. */
    retained = (uint16_t)(sample_count - 2U);
    sum -= minimum;
    sum -= maximum;

    reading->average_code = (uint16_t)((sum + (retained / 2U)) / retained);
    reading->minimum_code = minimum;
    reading->maximum_code = maximum;
    reading->spread_code = (uint16_t)(maximum - minimum);
    reading->retained_samples = retained;

    if (reading->spread_code > SECDB_ADC_MAX_STABLE_SPREAD) {
        return fail_safe(SECDB_ADC_UNSTABLE);
    }
    return SECDB_ADC_OK;
}

void SECDB_ADC_InvalidateCalibration(void)
{
    calibration.low_average_code = 0U;
    calibration.high_average_code = 0U;
    calibration.low_spread_code = 0U;
    calibration.high_spread_code = 0U;
    calibration.measured_span_code = 0U;
    calibration.valid = false;
    SECDB_AFE_SetMeasurementPermit(false);
}

secdb_adc_status_t SECDB_ADC_RunCalibration(secdb_adc_calibration_t *result)
{
    secdb_adc_reading_t low;
    secdb_adc_reading_t high;
    secdb_adc_status_t status;

    SECDB_ADC_InvalidateCalibration();
    if (result != NULL) {
        *result = calibration;
    }
    if (adc_handle == NULL) {
        return SECDB_ADC_NOT_INITIALIZED;
    }

    if (SECDB_AFE_ApplyMode(SECDB_AFE_CAL_1V1) != SECDB_STATUS_OK) {
        return fail_safe(SECDB_ADC_HAL_ERROR);
    }
    status = SECDB_ADC_ReadAveraged(SECDB_ADC_DEFAULT_SAMPLE_COUNT,
                                    SECDB_ADC_DEFAULT_TIMEOUT_MS, &low);
    if (status != SECDB_ADC_OK) {
        return status;
    }
    calibration.low_average_code = low.average_code;
    calibration.low_spread_code = low.spread_code;

    if (SECDB_AFE_ApplyMode(SECDB_AFE_CAL_2V2) != SECDB_STATUS_OK) {
        return fail_safe(SECDB_ADC_HAL_ERROR);
    }
    status = SECDB_ADC_ReadAveraged(SECDB_ADC_DEFAULT_SAMPLE_COUNT,
                                    SECDB_ADC_DEFAULT_TIMEOUT_MS, &high);
    SECDB_AFE_EnterSafeTransition();
    if (status != SECDB_ADC_OK) {
        if (result != NULL) {
            *result = calibration;
        }
        return status;
    }

    calibration.high_average_code = high.average_code;
    calibration.high_spread_code = high.spread_code;

    if (!in_window(low.average_code, SECDB_ADC_CAL_LOW_IDEAL_CODE,
                   SECDB_ADC_CAL_CODE_TOLERANCE)) {
        status = SECDB_ADC_CAL_LOW_OUT_OF_RANGE;
    } else if (!in_window(high.average_code, SECDB_ADC_CAL_HIGH_IDEAL_CODE,
                          SECDB_ADC_CAL_CODE_TOLERANCE)) {
        status = SECDB_ADC_CAL_HIGH_OUT_OF_RANGE;
    } else if (high.average_code <= low.average_code) {
        status = SECDB_ADC_CAL_NON_MONOTONIC;
    } else {
        calibration.measured_span_code =
            (uint16_t)(high.average_code - low.average_code);
        status = in_window(calibration.measured_span_code,
                           SECDB_ADC_CAL_SPAN_IDEAL_CODE,
                           SECDB_ADC_CAL_SPAN_TOLERANCE)
                     ? SECDB_ADC_OK : SECDB_ADC_CAL_SPAN_OUT_OF_RANGE;
    }

    if (status != SECDB_ADC_OK) {
        if (result != NULL) {
            *result = calibration;
        }
        return fail_safe(status);
    }

    calibration.valid = true;
    SECDB_AFE_SetMeasurementPermit(true);

    /* CAL_1V1 is the safe connected idle state after a successful self-test. */
    if (SECDB_AFE_ApplyMode(SECDB_AFE_CAL_1V1) != SECDB_STATUS_OK) {
        SECDB_ADC_InvalidateCalibration();
        return SECDB_ADC_HAL_ERROR;
    }

    if (result != NULL) {
        *result = calibration;
    }
    return SECDB_ADC_OK;
}

bool SECDB_ADC_IsCalibrationValid(void)
{
    return calibration.valid;
}

secdb_adc_calibration_t SECDB_ADC_GetCalibration(void)
{
    return calibration;
}

secdb_adc_status_t SECDB_ADC_CorrectCode(uint16_t raw_code,
                                         uint16_t *corrected_code)
{
    int32_t raw_delta;
    int64_t scaled_delta;
    int32_t corrected;

    if (corrected_code == NULL) {
        return SECDB_ADC_INVALID_ARGUMENT;
    }
    if (!calibration.valid || (calibration.measured_span_code == 0U)) {
        return SECDB_ADC_CALIBRATION_REQUIRED;
    }

    raw_delta = (int32_t)raw_code - (int32_t)calibration.low_average_code;
    scaled_delta = (int64_t)raw_delta * (int64_t)SECDB_ADC_CAL_SPAN_IDEAL_CODE;
    if (scaled_delta >= 0) {
        scaled_delta += (int64_t)calibration.measured_span_code / 2;
    } else {
        scaled_delta -= (int64_t)calibration.measured_span_code / 2;
    }

    corrected = (int32_t)SECDB_ADC_CAL_LOW_IDEAL_CODE +
                (int32_t)(scaled_delta / calibration.measured_span_code);
    if (corrected < 0) {
        corrected = 0;
    } else if (corrected > (int32_t)SECDB_ADC_FULL_SCALE_CODE) {
        corrected = SECDB_ADC_FULL_SCALE_CODE;
    }

    *corrected_code = (uint16_t)corrected;
    return SECDB_ADC_OK;
}

secdb_adc_status_t SECDB_ADC_ExpectedRawCode(uint16_t corrected_target_code,
                                             uint16_t *raw_target_code)
{
    int32_t corrected_delta;
    int64_t scaled_delta;
    int32_t raw_target;

    if ((raw_target_code == NULL) ||
        (corrected_target_code > SECDB_ADC_FULL_SCALE_CODE)) {
        return SECDB_ADC_INVALID_ARGUMENT;
    }
    if (!calibration.valid || (calibration.measured_span_code == 0U)) {
        return SECDB_ADC_CALIBRATION_REQUIRED;
    }

    corrected_delta = (int32_t)corrected_target_code -
                      (int32_t)SECDB_ADC_CAL_LOW_IDEAL_CODE;
    scaled_delta = (int64_t)corrected_delta * calibration.measured_span_code;
    if (scaled_delta >= 0) {
        scaled_delta += SECDB_ADC_CAL_SPAN_IDEAL_CODE / 2U;
    } else {
        scaled_delta -= SECDB_ADC_CAL_SPAN_IDEAL_CODE / 2U;
    }

    raw_target = (int32_t)calibration.low_average_code +
                 (int32_t)(scaled_delta / SECDB_ADC_CAL_SPAN_IDEAL_CODE);
    if (raw_target < 0) {
        raw_target = 0;
    } else if (raw_target > (int32_t)SECDB_ADC_FULL_SCALE_CODE) {
        raw_target = SECDB_ADC_FULL_SCALE_CODE;
    }

    *raw_target_code = (uint16_t)raw_target;
    return SECDB_ADC_OK;
}

secdb_adc_status_t SECDB_ADC_ReadCorrectedAveraged(
    uint16_t sample_count,
    uint32_t timeout_ms,
    secdb_adc_reading_t *reading,
    uint16_t *corrected_code)
{
    secdb_adc_status_t status;

    if ((reading == NULL) || (corrected_code == NULL)) {
        return fail_safe(SECDB_ADC_INVALID_ARGUMENT);
    }
    if (!calibration.valid) {
        return fail_safe(SECDB_ADC_CALIBRATION_REQUIRED);
    }

    status = SECDB_ADC_ReadAveraged(sample_count, timeout_ms, reading);
    if (status != SECDB_ADC_OK) {
        return status;
    }
    return SECDB_ADC_CorrectCode(reading->average_code, corrected_code);
}
