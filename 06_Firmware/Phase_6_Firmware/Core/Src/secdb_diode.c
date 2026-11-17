#include "secdb_diode.h"
#include <stddef.h>

typedef struct {
    uint16_t ab_a;
    uint16_t ab_b;
    uint16_t ba_a;
    uint16_t ba_b;
} diode_codes_t;

static uint16_t absolute_difference(uint16_t a, uint16_t b)
{
    return (a >= b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static bool near_mv(uint16_t value, uint16_t boundary)
{
    return absolute_difference(value, boundary) <= SECDB_DIODE_TYPE_GUARD_MV;
}

static secdb_diode_type_t classify_type(uint16_t forward_mv)
{
    if (near_mv(forward_mv, 50U) || near_mv(forward_mv, 450U) ||
        near_mv(forward_mv, 500U) || near_mv(forward_mv, 900U) ||
        near_mv(forward_mv, 1200U) || near_mv(forward_mv, 2900U) ||
        ((forward_mv >= 450U) && (forward_mv <= 500U))) {
        return SECDB_DIODE_TYPE_UNCERTAIN;
    }
    if ((forward_mv > 70U) && (forward_mv < 430U)) {
        return SECDB_DIODE_TYPE_SCHOTTKY_LIKE;
    }
    if ((forward_mv > 520U) && (forward_mv < 880U)) {
        return SECDB_DIODE_TYPE_SILICON_LIKE;
    }
    if ((forward_mv > 1220U) && (forward_mv < 2880U)) {
        return SECDB_DIODE_TYPE_LED_LIKE;
    }
    return SECDB_DIODE_TYPE_UNCERTAIN;
}

static secdb_diode_status_t read_terminal(secdb_afe_mode_t mode,
                                           uint16_t *corrected_code,
                                           secdb_adc_status_t *adc_status,
                                           secdb_status_t *afe_status)
{
    secdb_adc_reading_t reading;

    *afe_status = SECDB_AFE_ApplyMode(mode);
    if (*afe_status != SECDB_STATUS_OK) {
        return SECDB_DIODE_AFE_FAILURE;
    }

    *adc_status = SECDB_ADC_ReadCorrectedAveraged(
        SECDB_ADC_DEFAULT_SAMPLE_COUNT,
        SECDB_ADC_DEFAULT_TIMEOUT_MS,
        &reading,
        corrected_code);
    return (*adc_status == SECDB_ADC_OK) ? SECDB_DIODE_CALIBRATION_READY
                                         : SECDB_DIODE_ADC_FAILURE;
}

static secdb_diode_status_t read_both_polarities(
    diode_codes_t *codes,
    secdb_adc_status_t *adc_status,
    secdb_status_t *afe_status)
{
    secdb_diode_status_t status;

    status = read_terminal(SECDB_AFE_DIODE_AB_READ_A, &codes->ab_a,
                           adc_status, afe_status);
    if (status != SECDB_DIODE_CALIBRATION_READY) {
        return status;
    }
    status = read_terminal(SECDB_AFE_DIODE_AB_READ_B, &codes->ab_b,
                           adc_status, afe_status);
    if (status != SECDB_DIODE_CALIBRATION_READY) {
        return status;
    }
    status = read_terminal(SECDB_AFE_DIODE_BA_READ_A, &codes->ba_a,
                           adc_status, afe_status);
    if (status != SECDB_DIODE_CALIBRATION_READY) {
        return status;
    }
    return read_terminal(SECDB_AFE_DIODE_BA_READ_B, &codes->ba_b,
                         adc_status, afe_status);
}

secdb_diode_status_t SECDB_Diode_RunOpenCalibration(
    uint16_t adc_full_scale_mv,
    secdb_diode_open_calibration_t *calibration)
{
    diode_codes_t codes = {0};
    secdb_adc_status_t adc_status = SECDB_ADC_OK;
    secdb_status_t afe_status = SECDB_STATUS_OK;
    secdb_diode_status_t status;

    if (calibration == NULL) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_DIODE_INVALID_ARGUMENT;
    }
    calibration->open_diff_ab_code = 0U;
    calibration->open_diff_ba_code = 0U;
    calibration->block_ab_code = 0U;
    calibration->block_ba_code = 0U;
    calibration->adc_full_scale_mv = adc_full_scale_mv;
    calibration->valid = false;

    if ((adc_full_scale_mv < 3000U) || (adc_full_scale_mv > 3600U)) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_DIODE_INVALID_ARGUMENT;
    }
    if (!SECDB_ADC_IsCalibrationValid()) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_DIODE_ADC_CALIBRATION_REQUIRED;
    }

    status = read_both_polarities(&codes, &adc_status, &afe_status);
    SECDB_AFE_EnterSafeTransition();
    if (status != SECDB_DIODE_CALIBRATION_READY) {
        return status;
    }

    calibration->open_diff_ab_code = absolute_difference(codes.ab_a, codes.ab_b);
    calibration->open_diff_ba_code = absolute_difference(codes.ba_b, codes.ba_a);
    if ((calibration->open_diff_ab_code < SECDB_DIODE_OPEN_CAL_MIN_CODE) ||
        (calibration->open_diff_ba_code < SECDB_DIODE_OPEN_CAL_MIN_CODE)) {
        return SECDB_DIODE_OPEN_CALIBRATION_FAILED;
    }

    calibration->block_ab_code = (uint16_t)(
        ((uint32_t)calibration->open_diff_ab_code *
         SECDB_DIODE_BLOCK_PERCENT + 50UL) / 100UL);
    calibration->block_ba_code = (uint16_t)(
        ((uint32_t)calibration->open_diff_ba_code *
         SECDB_DIODE_BLOCK_PERCENT + 50UL) / 100UL);
    calibration->valid = true;
    return SECDB_DIODE_CALIBRATION_READY;
}

secdb_diode_status_t SECDB_Diode_Measure(
    const secdb_diode_open_calibration_t *calibration,
    secdb_diode_result_t *result)
{
    diode_codes_t codes = {0};
    secdb_diode_status_t read_status;

    if (result == NULL) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_DIODE_INVALID_ARGUMENT;
    }
    result->status = SECDB_DIODE_INVALID_ARGUMENT;
    result->type = SECDB_DIODE_TYPE_UNCERTAIN;
    result->ab_terminal_a_code = 0U;
    result->ab_terminal_b_code = 0U;
    result->ba_terminal_a_code = 0U;
    result->ba_terminal_b_code = 0U;
    result->diff_ab_code = 0U;
    result->diff_ba_code = 0U;
    result->forward_code = 0U;
    result->forward_mv = 0U;
    result->adc_status = SECDB_ADC_OK;
    result->afe_status = SECDB_STATUS_OK;

    if (!SECDB_ADC_IsCalibrationValid()) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_DIODE_ADC_CALIBRATION_REQUIRED;
        return result->status;
    }
    if ((calibration == NULL) || !calibration->valid) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_DIODE_OPEN_CALIBRATION_REQUIRED;
        return result->status;
    }
    if ((calibration->adc_full_scale_mv < 3000U) ||
        (calibration->adc_full_scale_mv > 3600U) ||
        (calibration->open_diff_ab_code < SECDB_DIODE_OPEN_CAL_MIN_CODE) ||
        (calibration->open_diff_ba_code < SECDB_DIODE_OPEN_CAL_MIN_CODE) ||
        (calibration->block_ab_code == 0U) ||
        (calibration->block_ba_code == 0U) ||
        (calibration->block_ab_code > calibration->open_diff_ab_code) ||
        (calibration->block_ba_code > calibration->open_diff_ba_code)) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_DIODE_INVALID_ARGUMENT;
        return result->status;
    }

    read_status = read_both_polarities(
        &codes, &result->adc_status, &result->afe_status);
    SECDB_AFE_EnterSafeTransition();
    if (read_status != SECDB_DIODE_CALIBRATION_READY) {
        result->status = read_status;
        return result->status;
    }

    result->ab_terminal_a_code = codes.ab_a;
    result->ab_terminal_b_code = codes.ab_b;
    result->ba_terminal_a_code = codes.ba_a;
    result->ba_terminal_b_code = codes.ba_b;
    result->diff_ab_code = absolute_difference(codes.ab_a, codes.ab_b);
    result->diff_ba_code = absolute_difference(codes.ba_b, codes.ba_a);

    if ((result->diff_ab_code >= calibration->block_ab_code) &&
        (result->diff_ba_code >= calibration->block_ba_code)) {
        result->status = SECDB_DIODE_OPEN_OR_NO_CONTACT;
    } else if ((result->diff_ab_code <= SECDB_DIODE_SHORT_CODE) &&
               (result->diff_ba_code <= SECDB_DIODE_SHORT_CODE)) {
        result->status = SECDB_DIODE_SHORT;
    } else if ((result->diff_ab_code < calibration->block_ab_code) &&
               (result->diff_ba_code >= calibration->block_ba_code)) {
        result->status = SECDB_DIODE_OK_ANODE_A;
        result->forward_code = result->diff_ab_code;
    } else if ((result->diff_ab_code >= calibration->block_ab_code) &&
               (result->diff_ba_code < calibration->block_ba_code)) {
        result->status = SECDB_DIODE_OK_ANODE_B;
        result->forward_code = result->diff_ba_code;
    } else {
        result->status = SECDB_DIODE_LEAKY_BIDIRECTIONAL_OR_UNKNOWN;
    }

    if ((result->status == SECDB_DIODE_OK_ANODE_A) ||
        (result->status == SECDB_DIODE_OK_ANODE_B)) {
        result->forward_mv = (uint16_t)(
            ((uint32_t)result->forward_code *
             calibration->adc_full_scale_mv +
             (SECDB_ADC_FULL_SCALE_CODE / 2U)) /
            SECDB_ADC_FULL_SCALE_CODE);
        result->type = classify_type(result->forward_mv);
    }
    return result->status;
}
