#ifndef SECDB_RESISTOR_H
#define SECDB_RESISTOR_H

#include "secdb_adc.h"
#include "secdb_afe.h"
#include <stdint.h>

#define SECDB_RESISTOR_TRIAL_LOW_CODE       1000U
#define SECDB_RESISTOR_TRIAL_HIGH_CODE      3100U
#define SECDB_RESISTOR_SHORT_CODE           20U
#define SECDB_RESISTOR_OPEN_CODE            4075U
#define SECDB_RESISTOR_MIN_SUPPORTED_OHM    100UL
#define SECDB_RESISTOR_MAX_SUPPORTED_OHM    1000000UL

typedef enum {
    SECDB_RES_RANGE_1K = 0,
    SECDB_RES_RANGE_10K,
    SECDB_RES_RANGE_100K
} secdb_resistor_range_t;

typedef enum {
    SECDB_RESISTOR_OK = 0,
    SECDB_RESISTOR_SHORT_OR_BELOW_MEASURABLE,
    SECDB_RESISTOR_OPEN_OR_ABOVE_MEASURABLE,
    SECDB_RESISTOR_BELOW_SUPPORTED_RANGE,
    SECDB_RESISTOR_ABOVE_SUPPORTED_RANGE,
    SECDB_RESISTOR_CALIBRATION_REQUIRED,
    SECDB_RESISTOR_INVALID_ARGUMENT,
    SECDB_RESISTOR_AFE_FAILURE,
    SECDB_RESISTOR_ADC_FAILURE
} secdb_resistor_status_t;

typedef struct {
    uint32_t path_1k_milliohm;
    uint32_t path_10k_milliohm;
    uint32_t path_100k_milliohm;
} secdb_resistor_calibration_t;

typedef struct {
    secdb_resistor_status_t status;
    secdb_resistor_range_t selected_range;
    uint16_t trial_code;
    uint16_t final_code;
    uint32_t resistance_ohm;
    secdb_adc_status_t adc_status;
    secdb_status_t afe_status;
} secdb_resistor_result_t;

/* Phase 5C simulation constants; replace with measured path values in Phase 7. */
secdb_resistor_calibration_t SECDB_Resistor_DefaultCalibration(void);

secdb_resistor_status_t SECDB_Resistor_Measure(
    const secdb_resistor_calibration_t *path_calibration,
    secdb_resistor_result_t *result);

#endif /* SECDB_RESISTOR_H */
