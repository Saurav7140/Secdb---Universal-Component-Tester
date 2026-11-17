#include "secdb_capacitor.h"
#include "secdb_timebase.h"
#include <stddef.h>

#define SECDB_CAP_DEFAULT_TIMING_DENOMINATOR_MILLIOHM  10352704UL

static secdb_capacitor_status_t discharge_until(
    uint16_t raw_discharge_threshold,
    uint32_t timeout_us,
    secdb_capacitor_result_t *result)
{
    uint32_t start_us;
    uint32_t elapsed_us;
    uint16_t discarded;

    result->afe_status = SECDB_AFE_ApplyMode(SECDB_AFE_CAP_DISCHARGE);
    if (result->afe_status != SECDB_STATUS_OK) {
        return SECDB_CAP_AFE_FAILURE;
    }

    result->adc_status = SECDB_ADC_ReadRaw(
        SECDB_ADC_DEFAULT_TIMEOUT_MS, &discarded);
    if (result->adc_status != SECDB_ADC_OK) {
        return SECDB_CAP_ADC_FAILURE;
    }

    start_us = SECDB_TimeUs();
    do {
        result->adc_status = SECDB_ADC_ReadRaw(
            SECDB_ADC_DEFAULT_TIMEOUT_MS, &result->last_raw_code);
        if (result->adc_status != SECDB_ADC_OK) {
            return SECDB_CAP_ADC_FAILURE;
        }
        elapsed_us = (uint32_t)(SECDB_TimeUs() - start_us);
        if ((result->last_raw_code <= raw_discharge_threshold) &&
            (elapsed_us <= timeout_us)) {
            return SECDB_CAP_OK;
        }
    } while (elapsed_us < timeout_us);

    return SECDB_CAP_INITIAL_DISCHARGE_FAILED_OR_EXTERNAL_VOLTAGE;
}

static secdb_capacitor_status_t finish_with_discharge(
    secdb_capacitor_status_t measurement_status,
    secdb_capacitor_result_t *result)
{
    const secdb_capacitor_status_t discharge_status = discharge_until(
        result->raw_discharge_threshold, SECDB_CAP_TIMEOUT_US, result);

    result->measurement_status = measurement_status;
    result->final_discharge_confirmed = (discharge_status == SECDB_CAP_OK);
    SECDB_AFE_EnterSafeTransition();

    if (!result->final_discharge_confirmed) {
        result->status = SECDB_CAP_FINAL_DISCHARGE_FAILED;
        return result->status;
    }

    result->status = measurement_status;
    return result->status;
}

uint32_t SECDB_Capacitor_DefaultTimingDenominatorMilliohm(void)
{
    return SECDB_CAP_DEFAULT_TIMING_DENOMINATOR_MILLIOHM;
}

secdb_capacitor_status_t SECDB_Capacitor_Measure(
    uint32_t timing_denominator_milliohm,
    secdb_capacitor_result_t *result)
{
    secdb_capacitor_status_t measurement_status;
    uint16_t discarded;
    uint32_t charge_start_us;
    uint64_t numerator;

    if (result == NULL) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_CAP_INVALID_ARGUMENT;
    }

    result->status = SECDB_CAP_INVALID_ARGUMENT;
    result->measurement_status = SECDB_CAP_INVALID_ARGUMENT;
    result->raw_discharge_threshold = 0U;
    result->raw_charge_threshold = 0U;
    result->last_raw_code = 0U;
    result->charge_time_us = 0UL;
    result->capacitance_nf = 0UL;
    result->final_discharge_confirmed = false;
    result->adc_status = SECDB_ADC_OK;
    result->afe_status = SECDB_STATUS_OK;

    if ((timing_denominator_milliohm <
         SECDB_CAP_MIN_TIMING_DENOMINATOR_MOHM) ||
        (timing_denominator_milliohm >
         SECDB_CAP_MAX_TIMING_DENOMINATOR_MOHM)) {
        SECDB_AFE_EnterSafeTransition();
        return SECDB_CAP_INVALID_ARGUMENT;
    }
    if (!SECDB_ADC_IsCalibrationValid()) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_CALIBRATION_REQUIRED;
        result->measurement_status = result->status;
        return result->status;
    }

    result->adc_status = SECDB_ADC_ExpectedRawCode(
        SECDB_CAP_DISCHARGE_THRESHOLD_CODE,
        &result->raw_discharge_threshold);
    if (result->adc_status != SECDB_ADC_OK) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_ADC_FAILURE;
        result->measurement_status = result->status;
        return result->status;
    }
    result->adc_status = SECDB_ADC_ExpectedRawCode(
        SECDB_CAP_CHARGE_THRESHOLD_CODE,
        &result->raw_charge_threshold);
    if (result->adc_status != SECDB_ADC_OK) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_ADC_FAILURE;
        result->measurement_status = result->status;
        return result->status;
    }

    measurement_status = discharge_until(
        result->raw_discharge_threshold, SECDB_CAP_TIMEOUT_US, result);
    if (measurement_status != SECDB_CAP_OK) {
        SECDB_AFE_EnterSafeTransition();
        result->status = measurement_status;
        result->measurement_status = measurement_status;
        return result->status;
    }

    result->afe_status = SECDB_AFE_ApplyMode(SECDB_AFE_CAP_READY);
    if (result->afe_status != SECDB_STATUS_OK) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_AFE_FAILURE;
        result->measurement_status = result->status;
        return result->status;
    }

    /* Discard once after the sense transition, then verify no voltage rebound. */
    result->adc_status = SECDB_ADC_ReadRaw(
        SECDB_ADC_DEFAULT_TIMEOUT_MS, &discarded);
    if (result->adc_status == SECDB_ADC_OK) {
        result->adc_status = SECDB_ADC_ReadRaw(
            SECDB_ADC_DEFAULT_TIMEOUT_MS, &result->last_raw_code);
    }
    if (result->adc_status != SECDB_ADC_OK) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_ADC_FAILURE;
        result->measurement_status = result->status;
        return result->status;
    }
    if (result->last_raw_code > result->raw_discharge_threshold) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_INITIAL_DISCHARGE_FAILED_OR_EXTERNAL_VOLTAGE;
        result->measurement_status = result->status;
        return result->status;
    }

    result->afe_status = SECDB_AFE_StartCapCharge(&charge_start_us);
    if (result->afe_status != SECDB_STATUS_OK) {
        SECDB_AFE_EnterSafeTransition();
        result->status = SECDB_CAP_AFE_FAILURE;
        result->measurement_status = result->status;
        return result->status;
    }

    do {
        result->adc_status = SECDB_ADC_ReadRaw(
            SECDB_ADC_DEFAULT_TIMEOUT_MS, &result->last_raw_code);
        if (result->adc_status != SECDB_ADC_OK) {
            return finish_with_discharge(SECDB_CAP_ADC_FAILURE, result);
        }
        result->charge_time_us = (uint32_t)(SECDB_TimeUs() - charge_start_us);
        if (result->last_raw_code >= result->raw_charge_threshold) {
            break;
        }
    } while (result->charge_time_us < SECDB_CAP_TIMEOUT_US);

    if ((result->last_raw_code < result->raw_charge_threshold) ||
        (result->charge_time_us > SECDB_CAP_TIMEOUT_US)) {
        measurement_status = SECDB_CAP_TIMEOUT_SHORT_LEAKAGE_OR_ABOVE_RANGE;
    } else if (result->charge_time_us < SECDB_CAP_MIN_CHARGE_TIME_US) {
        measurement_status = SECDB_CAP_OPEN_OR_BELOW_100NF;
    } else {
        numerator = (uint64_t)result->charge_time_us * 1000000ULL;
        result->capacitance_nf = (uint32_t)(
            (numerator + (timing_denominator_milliohm / 2UL)) /
            timing_denominator_milliohm);

        if (result->capacitance_nf < SECDB_CAP_MIN_SUPPORTED_NF) {
            measurement_status = SECDB_CAP_OPEN_OR_BELOW_100NF;
        } else if (result->capacitance_nf > SECDB_CAP_MAX_SUPPORTED_NF) {
            measurement_status = SECDB_CAP_ABOVE_100UF;
        } else {
            measurement_status = SECDB_CAP_OK;
        }
    }

    return finish_with_discharge(measurement_status, result);
}
