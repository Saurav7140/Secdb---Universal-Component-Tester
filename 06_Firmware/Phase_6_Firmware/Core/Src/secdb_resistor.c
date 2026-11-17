#include "secdb_resistor.h"
#include <stddef.h>

#define SECDB_PATH_1K_DEFAULT_MILLIOHM      1340500UL
#define SECDB_PATH_10K_DEFAULT_MILLIOHM    10349500UL
#define SECDB_PATH_100K_DEFAULT_MILLIOHM   100439500UL

static bool calibration_is_plausible(
    const secdb_resistor_calibration_t *calibration)
{
    return (calibration != NULL) &&
           (calibration->path_1k_milliohm >= 500000UL) &&
           (calibration->path_1k_milliohm <= 5000000UL) &&
           (calibration->path_10k_milliohm >= 5000000UL) &&
           (calibration->path_10k_milliohm <= 50000000UL) &&
           (calibration->path_100k_milliohm >= 50000000UL) &&
           (calibration->path_100k_milliohm <= 500000000UL);
}

static secdb_resistor_status_t finish(secdb_resistor_result_t *result,
                                      secdb_resistor_status_t status)
{
    SECDB_AFE_EnterSafeTransition();
    result->status = status;
    return status;
}

static uint32_t path_for_range(const secdb_resistor_calibration_t *calibration,
                               secdb_resistor_range_t range)
{
    switch (range) {
    case SECDB_RES_RANGE_1K:
        return calibration->path_1k_milliohm;
    case SECDB_RES_RANGE_10K:
        return calibration->path_10k_milliohm;
    case SECDB_RES_RANGE_100K:
        return calibration->path_100k_milliohm;
    default:
        return 0UL;
    }
}

static secdb_afe_mode_t mode_for_range(secdb_resistor_range_t range)
{
    switch (range) {
    case SECDB_RES_RANGE_1K:
        return SECDB_AFE_RESISTOR_1K;
    case SECDB_RES_RANGE_100K:
        return SECDB_AFE_RESISTOR_100K;
    case SECDB_RES_RANGE_10K:
    default:
        return SECDB_AFE_RESISTOR_10K_TRIAL;
    }
}

static uint32_t calculate_resistance_ohm(uint32_t path_milliohm,
                                         uint16_t corrected_code)
{
    const uint32_t denominator_code =
        SECDB_ADC_FULL_SCALE_CODE - corrected_code;
    const uint64_t denominator = (uint64_t)denominator_code * 1000ULL;
    const uint64_t numerator = (uint64_t)path_milliohm * corrected_code;
    const uint64_t rounded = (numerator + (denominator / 2ULL)) / denominator;

    return (rounded > UINT32_MAX) ? UINT32_MAX : (uint32_t)rounded;
}

secdb_resistor_calibration_t SECDB_Resistor_DefaultCalibration(void)
{
    const secdb_resistor_calibration_t calibration = {
        SECDB_PATH_1K_DEFAULT_MILLIOHM,
        SECDB_PATH_10K_DEFAULT_MILLIOHM,
        SECDB_PATH_100K_DEFAULT_MILLIOHM
    };
    return calibration;
}

secdb_resistor_status_t SECDB_Resistor_Measure(
    const secdb_resistor_calibration_t *path_calibration,
    secdb_resistor_result_t *result)
{
    secdb_adc_reading_t reading;
    secdb_adc_status_t adc_status;
    secdb_status_t afe_status;
    uint32_t selected_path;

    if (result == NULL) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_RESISTOR_INVALID_ARGUMENT;
    }

    result->status = SECDB_RESISTOR_INVALID_ARGUMENT;
    result->selected_range = SECDB_RES_RANGE_10K;
    result->trial_code = 0U;
    result->final_code = 0U;
    result->resistance_ohm = 0UL;
    result->adc_status = SECDB_ADC_OK;
    result->afe_status = SECDB_STATUS_OK;

    if (!calibration_is_plausible(path_calibration)) {
        return finish(result, SECDB_RESISTOR_INVALID_ARGUMENT);
    }
    if (!SECDB_ADC_IsCalibrationValid()) {
        return finish(result, SECDB_RESISTOR_CALIBRATION_REQUIRED);
    }

    afe_status = SECDB_AFE_ApplyMode(SECDB_AFE_RESISTOR_10K_TRIAL);
    result->afe_status = afe_status;
    if (afe_status != SECDB_STATUS_OK) {
        return finish(result, SECDB_RESISTOR_AFE_FAILURE);
    }

    adc_status = SECDB_ADC_ReadCorrectedAveraged(
        SECDB_ADC_DEFAULT_SAMPLE_COUNT,
        SECDB_ADC_DEFAULT_TIMEOUT_MS,
        &reading,
        &result->trial_code);
    result->adc_status = adc_status;
    if (adc_status != SECDB_ADC_OK) {
        return finish(result, SECDB_RESISTOR_ADC_FAILURE);
    }

    if (result->trial_code < SECDB_RESISTOR_TRIAL_LOW_CODE) {
        result->selected_range = SECDB_RES_RANGE_1K;
    } else if (result->trial_code > SECDB_RESISTOR_TRIAL_HIGH_CODE) {
        result->selected_range = SECDB_RES_RANGE_100K;
    } else {
        result->selected_range = SECDB_RES_RANGE_10K;
    }

    afe_status = SECDB_AFE_ApplyMode(mode_for_range(result->selected_range));
    result->afe_status = afe_status;
    if (afe_status != SECDB_STATUS_OK) {
        return finish(result, SECDB_RESISTOR_AFE_FAILURE);
    }

    adc_status = SECDB_ADC_ReadCorrectedAveraged(
        SECDB_ADC_DEFAULT_SAMPLE_COUNT,
        SECDB_ADC_DEFAULT_TIMEOUT_MS,
        &reading,
        &result->final_code);
    result->adc_status = adc_status;
    if (adc_status != SECDB_ADC_OK) {
        return finish(result, SECDB_RESISTOR_ADC_FAILURE);
    }

    if (result->final_code < SECDB_RESISTOR_SHORT_CODE) {
        return finish(result, SECDB_RESISTOR_SHORT_OR_BELOW_MEASURABLE);
    }
    if (result->final_code > SECDB_RESISTOR_OPEN_CODE) {
        return finish(result, SECDB_RESISTOR_OPEN_OR_ABOVE_MEASURABLE);
    }

    selected_path = path_for_range(path_calibration, result->selected_range);
    result->resistance_ohm =
        calculate_resistance_ohm(selected_path, result->final_code);

    if (result->resistance_ohm < SECDB_RESISTOR_MIN_SUPPORTED_OHM) {
        return finish(result, SECDB_RESISTOR_BELOW_SUPPORTED_RANGE);
    }
    if (result->resistance_ohm > SECDB_RESISTOR_MAX_SUPPORTED_OHM) {
        return finish(result, SECDB_RESISTOR_ABOVE_SUPPORTED_RANGE);
    }
    return finish(result, SECDB_RESISTOR_OK);
}
