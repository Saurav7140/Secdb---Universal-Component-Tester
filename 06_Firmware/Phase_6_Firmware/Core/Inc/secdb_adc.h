#ifndef SECDB_ADC_H
#define SECDB_ADC_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define SECDB_ADC_FULL_SCALE_CODE         4095U
#define SECDB_ADC_DEFAULT_SAMPLE_COUNT    32U
#define SECDB_ADC_MIN_SAMPLE_COUNT        4U
#define SECDB_ADC_MAX_SAMPLE_COUNT        64U
#define SECDB_ADC_DEFAULT_TIMEOUT_MS      5U

#define SECDB_ADC_CAL_LOW_IDEAL_CODE      1365U
#define SECDB_ADC_CAL_HIGH_IDEAL_CODE     2730U
#define SECDB_ADC_CAL_CODE_TOLERANCE      64U
#define SECDB_ADC_CAL_SPAN_IDEAL_CODE     1365U
#define SECDB_ADC_CAL_SPAN_TOLERANCE      64U
#define SECDB_ADC_MAX_STABLE_SPREAD       16U

typedef enum {
    SECDB_ADC_OK = 0,
    SECDB_ADC_INVALID_ARGUMENT,
    SECDB_ADC_NOT_INITIALIZED,
    SECDB_ADC_AFE_NOT_CONNECTED,
    SECDB_ADC_HAL_ERROR,
    SECDB_ADC_TIMEOUT,
    SECDB_ADC_UNSTABLE,
    SECDB_ADC_CAL_LOW_OUT_OF_RANGE,
    SECDB_ADC_CAL_HIGH_OUT_OF_RANGE,
    SECDB_ADC_CAL_NON_MONOTONIC,
    SECDB_ADC_CAL_SPAN_OUT_OF_RANGE,
    SECDB_ADC_CALIBRATION_REQUIRED
} secdb_adc_status_t;

typedef struct {
    uint16_t average_code;
    uint16_t minimum_code;
    uint16_t maximum_code;
    uint16_t spread_code;
    uint16_t retained_samples;
} secdb_adc_reading_t;

typedef struct {
    uint16_t low_average_code;
    uint16_t high_average_code;
    uint16_t low_spread_code;
    uint16_t high_spread_code;
    uint16_t measured_span_code;
    bool valid;
} secdb_adc_calibration_t;

/* Bind the CubeMX-initialized ADC1 handle. This does not call HAL_ADC_Init(). */
secdb_adc_status_t SECDB_ADC_Bind(ADC_HandleTypeDef *hadc);

/* One conversion without averaging; intended for Phase 6C capacitor polling. */
secdb_adc_status_t SECDB_ADC_ReadRaw(uint32_t timeout_ms, uint16_t *code);

/* Discards one conversion, then returns a min/max-trimmed mean. */
secdb_adc_status_t SECDB_ADC_ReadAveraged(uint16_t sample_count,
                                          uint32_t timeout_ms,
                                          secdb_adc_reading_t *reading);

/* Reads both calibration taps, validates them, and unlocks DUT modes on pass. */
secdb_adc_status_t SECDB_ADC_RunCalibration(secdb_adc_calibration_t *result);

void SECDB_ADC_InvalidateCalibration(void);
bool SECDB_ADC_IsCalibrationValid(void);
secdb_adc_calibration_t SECDB_ADC_GetCalibration(void);

/* Apply the validated two-point offset/gain correction and clamp to 0..4095. */
secdb_adc_status_t SECDB_ADC_CorrectCode(uint16_t raw_code,
                                         uint16_t *corrected_code);

/* Inverse of the two-point correction; useful for fast raw-code thresholds. */
secdb_adc_status_t SECDB_ADC_ExpectedRawCode(uint16_t corrected_target_code,
                                             uint16_t *raw_target_code);

secdb_adc_status_t SECDB_ADC_ReadCorrectedAveraged(
    uint16_t sample_count,
    uint32_t timeout_ms,
    secdb_adc_reading_t *reading,
    uint16_t *corrected_code);

#endif /* SECDB_ADC_H */
