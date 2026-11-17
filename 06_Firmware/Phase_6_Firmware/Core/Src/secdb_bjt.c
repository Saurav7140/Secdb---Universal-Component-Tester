#include "secdb_bjt.h"
#include <stddef.h>

#define SECDB_BJT_DEFAULT_DRIVE_LOW_MV        0U
#define SECDB_BJT_DEFAULT_DRIVE_HIGH_MV    3300U
#define SECDB_BJT_DEFAULT_ADC_FS_MV        3300U
#define SECDB_BJT_DEFAULT_BASE_PATH_OHM  1001050UL
#define SECDB_BJT_DEFAULT_COL_PATH_OHM      1051UL

typedef enum {
    BJT_NODE_EMITTER = 0,
    BJT_NODE_BASE,
    BJT_NODE_COLLECTOR
} bjt_node_t;

static uint16_t positive_difference(uint16_t high, uint16_t low)
{
    return (high > low) ? (uint16_t)(high - low) : 0U;
}

static bool calibration_is_plausible(const secdb_bjt_calibration_t *calibration)
{
    return (calibration != NULL) &&
           (calibration->drive_low_mv <= 100U) &&
           (calibration->drive_high_mv >= 3000U) &&
           (calibration->drive_high_mv <= 3600U) &&
           (calibration->drive_high_mv > calibration->drive_low_mv) &&
           (calibration->adc_full_scale_mv >= 3000U) &&
           (calibration->adc_full_scale_mv <= 3600U) &&
           (calibration->base_path_ohm >= 500000UL) &&
           (calibration->base_path_ohm <= 2000000UL) &&
           (calibration->collector_path_ohm >= 500UL) &&
           (calibration->collector_path_ohm <= 2000UL);
}

static uint16_t code_to_mv(uint16_t code,
                           const secdb_bjt_calibration_t *calibration)
{
    return (uint16_t)(
        ((uint32_t)code * calibration->adc_full_scale_mv +
         (SECDB_ADC_FULL_SCALE_CODE / 2U)) /
        SECDB_ADC_FULL_SCALE_CODE);
}

static uint32_t current_na(uint16_t voltage_drop_mv, uint32_t resistance_ohm)
{
    const uint64_t numerator = (uint64_t)voltage_drop_mv * 1000000ULL;
    return (uint32_t)((numerator + (resistance_ohm / 2UL)) / resistance_ohm);
}

static secdb_bjt_status_t finish(secdb_bjt_result_t *result,
                                 secdb_bjt_status_t status)
{
    SECDB_AFE_EnterSafeTransition();
    result->status = status;
    return status;
}

static secdb_afe_mode_t active_mode(secdb_bjt_polarity_t polarity,
                                    bjt_node_t node)
{
    if (polarity == SECDB_BJT_NPN) {
        if (node == BJT_NODE_EMITTER) {
            return SECDB_AFE_NPN_READ_E;
        }
        if (node == BJT_NODE_BASE) {
            return SECDB_AFE_NPN_READ_B;
        }
        return SECDB_AFE_NPN_READ_C;
    }

    if (node == BJT_NODE_EMITTER) {
        return SECDB_AFE_PNP_READ_E;
    }
    if (node == BJT_NODE_BASE) {
        return SECDB_AFE_PNP_READ_B;
    }
    return SECDB_AFE_PNP_READ_C;
}

static secdb_afe_mode_t base_off_mode(secdb_bjt_polarity_t polarity,
                                      bjt_node_t node)
{
    if (polarity == SECDB_BJT_NPN) {
        return (node == BJT_NODE_EMITTER)
             ? SECDB_AFE_NPN_BASE_OFF_READ_E
             : SECDB_AFE_NPN_BASE_OFF_READ_C;
    }
    return (node == BJT_NODE_EMITTER)
         ? SECDB_AFE_PNP_BASE_OFF_READ_E
         : SECDB_AFE_PNP_BASE_OFF_READ_C;
}

static secdb_bjt_status_t read_node(secdb_afe_mode_t mode,
                                    uint16_t *code,
                                    secdb_bjt_result_t *result)
{
    secdb_adc_reading_t reading;

    result->afe_status = SECDB_AFE_ApplyMode(mode);
    if (result->afe_status != SECDB_STATUS_OK) {
        return SECDB_BJT_AFE_FAILURE;
    }
    result->adc_status = SECDB_ADC_ReadCorrectedAveraged(
        SECDB_ADC_DEFAULT_SAMPLE_COUNT,
        SECDB_ADC_DEFAULT_TIMEOUT_MS,
        &reading,
        code);
    return (result->adc_status == SECDB_ADC_OK) ? SECDB_BJT_ACTIVE_INDICATIVE_HFE
                                                : SECDB_BJT_ADC_FAILURE;
}

static secdb_bjt_status_t perform_base_off_confirmation(
    const secdb_bjt_calibration_t *calibration,
    secdb_bjt_result_t *result)
{
    uint16_t emitter_code;
    uint16_t collector_code;
    uint16_t collector_drop_mv;
    uint32_t collector_current_na;
    secdb_bjt_status_t status;

    result->base_off_performed = true;
    status = read_node(base_off_mode(result->polarity, BJT_NODE_EMITTER),
                       &emitter_code, result);
    if (status != SECDB_BJT_ACTIVE_INDICATIVE_HFE) {
        return status;
    }
    status = read_node(base_off_mode(result->polarity, BJT_NODE_COLLECTOR),
                       &collector_code, result);
    if (status != SECDB_BJT_ACTIVE_INDICATIVE_HFE) {
        return status;
    }

    result->base_off_emitter_mv = code_to_mv(emitter_code, calibration);
    result->base_off_collector_mv = code_to_mv(collector_code, calibration);
    if (result->polarity == SECDB_BJT_NPN) {
        result->base_off_ce_voltage_mv = positive_difference(
            result->base_off_collector_mv, result->base_off_emitter_mv);
        collector_drop_mv = positive_difference(
            calibration->drive_high_mv, result->base_off_collector_mv);
    } else {
        result->base_off_ce_voltage_mv = positive_difference(
            result->base_off_emitter_mv, result->base_off_collector_mv);
        collector_drop_mv = positive_difference(
            result->base_off_collector_mv, calibration->drive_low_mv);
    }

    collector_current_na = current_na(
        collector_drop_mv, calibration->collector_path_ohm);
    result->base_off_collector_current_ua =
        (collector_current_na + 500UL) / 1000UL;

    if ((result->base_off_ce_voltage_mv <=
         SECDB_BJT_BASE_OFF_CE_SHORT_MAX_MV) &&
        (result->base_off_collector_current_ua >=
         SECDB_BJT_BASE_OFF_SHORT_MIN_UA)) {
        return SECDB_BJT_CE_SHORT_CANDIDATE;
    }
    return SECDB_BJT_SATURATION_OR_INVALID_ACTIVE_REGION;
}

secdb_bjt_calibration_t SECDB_BJT_DefaultCalibration(void)
{
    const secdb_bjt_calibration_t calibration = {
        SECDB_BJT_DEFAULT_DRIVE_LOW_MV,
        SECDB_BJT_DEFAULT_DRIVE_HIGH_MV,
        SECDB_BJT_DEFAULT_ADC_FS_MV,
        SECDB_BJT_DEFAULT_BASE_PATH_OHM,
        SECDB_BJT_DEFAULT_COL_PATH_OHM
    };
    return calibration;
}

secdb_bjt_status_t SECDB_BJT_Measure(
    secdb_bjt_polarity_t polarity,
    const secdb_bjt_calibration_t *calibration,
    secdb_bjt_result_t *result)
{
    secdb_bjt_status_t status;
    uint16_t base_drive_drop_mv;
    uint16_t collector_drive_drop_mv;
    uint32_t collector_current_na;

    if (result == NULL) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_BJT_INVALID_ARGUMENT;
    }

    result->status = SECDB_BJT_INVALID_ARGUMENT;
    result->polarity = polarity;
    result->emitter_code = 0U;
    result->base_code = 0U;
    result->collector_code = 0U;
    result->emitter_mv = 0U;
    result->base_mv = 0U;
    result->collector_mv = 0U;
    result->junction_mv = 0U;
    result->ce_voltage_mv = 0U;
    result->cb_voltage_mv = 0U;
    result->base_current_na = 0UL;
    result->collector_current_ua = 0UL;
    result->indicative_hfe_x10 = 0UL;
    result->hfe_valid = false;
    result->base_off_performed = false;
    result->base_off_emitter_mv = 0U;
    result->base_off_collector_mv = 0U;
    result->base_off_ce_voltage_mv = 0U;
    result->base_off_collector_current_ua = 0UL;
    result->adc_status = SECDB_ADC_OK;
    result->afe_status = SECDB_STATUS_OK;

    if ((polarity != SECDB_BJT_NPN) && (polarity != SECDB_BJT_PNP)) {
        return finish(result, SECDB_BJT_INVALID_ARGUMENT);
    }
    if (!calibration_is_plausible(calibration)) {
        return finish(result, SECDB_BJT_INVALID_ARGUMENT);
    }
    if (!SECDB_ADC_IsCalibrationValid()) {
        return finish(result, SECDB_BJT_CALIBRATION_REQUIRED);
    }

    status = read_node(active_mode(polarity, BJT_NODE_EMITTER),
                       &result->emitter_code, result);
    if (status != SECDB_BJT_ACTIVE_INDICATIVE_HFE) {
        return finish(result, status);
    }
    status = read_node(active_mode(polarity, BJT_NODE_BASE),
                       &result->base_code, result);
    if (status != SECDB_BJT_ACTIVE_INDICATIVE_HFE) {
        return finish(result, status);
    }
    status = read_node(active_mode(polarity, BJT_NODE_COLLECTOR),
                       &result->collector_code, result);
    if (status != SECDB_BJT_ACTIVE_INDICATIVE_HFE) {
        return finish(result, status);
    }

    result->emitter_mv = code_to_mv(result->emitter_code, calibration);
    result->base_mv = code_to_mv(result->base_code, calibration);
    result->collector_mv = code_to_mv(result->collector_code, calibration);

    if (polarity == SECDB_BJT_NPN) {
        result->junction_mv = positive_difference(
            result->base_mv, result->emitter_mv);
        result->ce_voltage_mv = positive_difference(
            result->collector_mv, result->emitter_mv);
        result->cb_voltage_mv = positive_difference(
            result->collector_mv, result->base_mv);
        base_drive_drop_mv = positive_difference(
            calibration->drive_high_mv, result->base_mv);
        collector_drive_drop_mv = positive_difference(
            calibration->drive_high_mv, result->collector_mv);
    } else {
        result->junction_mv = positive_difference(
            result->emitter_mv, result->base_mv);
        result->ce_voltage_mv = positive_difference(
            result->emitter_mv, result->collector_mv);
        result->cb_voltage_mv = positive_difference(
            result->base_mv, result->collector_mv);
        base_drive_drop_mv = positive_difference(
            result->base_mv, calibration->drive_low_mv);
        collector_drive_drop_mv = positive_difference(
            result->collector_mv, calibration->drive_low_mv);
    }

    result->base_current_na = current_na(
        base_drive_drop_mv, calibration->base_path_ohm);
    collector_current_na = current_na(
        collector_drive_drop_mv, calibration->collector_path_ohm);
    result->collector_current_ua = (collector_current_na + 500UL) / 1000UL;

    if (result->junction_mv < SECDB_BJT_BE_SHORT_MAX_MV) {
        return finish(result, SECDB_BJT_BE_SHORT_OR_WRONG_CONNECTION);
    }
    if (result->junction_mv > SECDB_BJT_JUNCTION_MAX_MV) {
        return finish(result, SECDB_BJT_OPEN_BASE_WRONG_PIN_OR_POLARITY);
    }
    if (result->junction_mv < SECDB_BJT_JUNCTION_MIN_MV) {
        return finish(result, SECDB_BJT_JUNCTION_OUT_OF_RANGE);
    }
    if (result->ce_voltage_mv <= SECDB_BJT_ACTIVE_CE_MIN_MV) {
        status = perform_base_off_confirmation(calibration, result);
        return finish(result, status);
    }
    if (result->cb_voltage_mv <= SECDB_BJT_ACTIVE_CB_MIN_MV) {
        return finish(result,
                      SECDB_BJT_BC_SHORT_SATURATION_OR_WRONG_CONNECTION);
    }
    if (collector_drive_drop_mv < SECDB_BJT_COLLECTOR_MARGIN_MIN_MV) {
        return finish(result, SECDB_BJT_OPEN_COLLECTOR_OR_NO_RESPONSE);
    }
    if ((base_drive_drop_mv < SECDB_BJT_BASE_DRIVE_MARGIN_MIN_MV) ||
        (result->base_current_na == 0UL)) {
        return finish(result, SECDB_BJT_LOW_GAIN_OR_INADEQUATE_MARGIN);
    }

    result->indicative_hfe_x10 = (uint32_t)(
        ((uint64_t)collector_current_na * 10ULL +
         (result->base_current_na / 2UL)) / result->base_current_na);

    if (result->indicative_hfe_x10 < SECDB_BJT_HFE_MIN_X10) {
        return finish(result, SECDB_BJT_LOW_GAIN_OR_INADEQUATE_MARGIN);
    }
    if (result->indicative_hfe_x10 > SECDB_BJT_HFE_MAX_X10) {
        return finish(result,
                      SECDB_BJT_ACTIVE_GAIN_ABOVE_CHARACTERIZED_RANGE);
    }

    result->hfe_valid = true;
    return finish(result, SECDB_BJT_ACTIVE_INDICATIVE_HFE);
}
