#ifndef SECDB_CAPACITOR_H
#define SECDB_CAPACITOR_H

#include "secdb_adc.h"
#include "secdb_afe.h"
#include <stdbool.h>
#include <stdint.h>

#define SECDB_CAP_CHARGE_THRESHOLD_CODE       2589U
#define SECDB_CAP_DISCHARGE_THRESHOLD_CODE    41U
#define SECDB_CAP_MIN_CHARGE_TIME_US           250UL
#define SECDB_CAP_TIMEOUT_US                   5000000UL
#define SECDB_CAP_MIN_SUPPORTED_NF             100UL
#define SECDB_CAP_MAX_SUPPORTED_NF             100000UL
#define SECDB_CAP_MIN_TIMING_DENOMINATOR_MOHM  1000000UL
#define SECDB_CAP_MAX_TIMING_DENOMINATOR_MOHM  100000000UL

typedef enum {
    SECDB_CAP_OK = 0,
    SECDB_CAP_OPEN_OR_BELOW_100NF,
    SECDB_CAP_ABOVE_100UF,
    SECDB_CAP_TIMEOUT_SHORT_LEAKAGE_OR_ABOVE_RANGE,
    SECDB_CAP_INITIAL_DISCHARGE_FAILED_OR_EXTERNAL_VOLTAGE,
    SECDB_CAP_FINAL_DISCHARGE_FAILED,
    SECDB_CAP_CALIBRATION_REQUIRED,
    SECDB_CAP_INVALID_ARGUMENT,
    SECDB_CAP_AFE_FAILURE,
    SECDB_CAP_ADC_FAILURE
} secdb_capacitor_status_t;

typedef struct {
    secdb_capacitor_status_t status;
    secdb_capacitor_status_t measurement_status;
    uint16_t raw_discharge_threshold;
    uint16_t raw_charge_threshold;
    uint16_t last_raw_code;
    uint32_t charge_time_us;
    uint32_t capacitance_nf;
    bool final_discharge_confirmed;
    secdb_adc_status_t adc_status;
    secdb_status_t afe_status;
} secdb_capacitor_result_t;

/* Phase 5C Rcharge*K threshold value; replace after physical calibration. */
uint32_t SECDB_Capacitor_DefaultTimingDenominatorMilliohm(void);

secdb_capacitor_status_t SECDB_Capacitor_Measure(
    uint32_t timing_denominator_milliohm,
    secdb_capacitor_result_t *result);

#endif /* SECDB_CAPACITOR_H */
