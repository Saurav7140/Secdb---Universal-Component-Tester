#ifndef SECDB_DIODE_H
#define SECDB_DIODE_H

#include "secdb_adc.h"
#include "secdb_afe.h"
#include <stdbool.h>
#include <stdint.h>

#define SECDB_DIODE_OPEN_CAL_MIN_CODE      3800U
#define SECDB_DIODE_BLOCK_PERCENT          90U
#define SECDB_DIODE_SHORT_CODE             124U
#define SECDB_DIODE_TYPE_GUARD_MV          20U

typedef enum {
    SECDB_DIODE_CALIBRATION_READY = 0,
    SECDB_DIODE_OK_ANODE_A,
    SECDB_DIODE_OK_ANODE_B,
    SECDB_DIODE_OPEN_OR_NO_CONTACT,
    SECDB_DIODE_SHORT,
    SECDB_DIODE_LEAKY_BIDIRECTIONAL_OR_UNKNOWN,
    SECDB_DIODE_ADC_CALIBRATION_REQUIRED,
    SECDB_DIODE_OPEN_CALIBRATION_REQUIRED,
    SECDB_DIODE_OPEN_CALIBRATION_FAILED,
    SECDB_DIODE_INVALID_ARGUMENT,
    SECDB_DIODE_AFE_FAILURE,
    SECDB_DIODE_ADC_FAILURE
} secdb_diode_status_t;

typedef enum {
    SECDB_DIODE_TYPE_UNCERTAIN = 0,
    SECDB_DIODE_TYPE_SCHOTTKY_LIKE,
    SECDB_DIODE_TYPE_SILICON_LIKE,
    SECDB_DIODE_TYPE_LED_LIKE
} secdb_diode_type_t;

typedef struct {
    uint16_t open_diff_ab_code;
    uint16_t open_diff_ba_code;
    uint16_t block_ab_code;
    uint16_t block_ba_code;
    uint16_t adc_full_scale_mv;
    bool valid;
} secdb_diode_open_calibration_t;

typedef struct {
    secdb_diode_status_t status;
    secdb_diode_type_t type;
    uint16_t ab_terminal_a_code;
    uint16_t ab_terminal_b_code;
    uint16_t ba_terminal_a_code;
    uint16_t ba_terminal_b_code;
    uint16_t diff_ab_code;
    uint16_t diff_ba_code;
    uint16_t forward_code;
    uint16_t forward_mv;
    secdb_adc_status_t adc_status;
    secdb_status_t afe_status;
} secdb_diode_result_t;

/* Must be called with the diode socket confirmed empty. */
secdb_diode_status_t SECDB_Diode_RunOpenCalibration(
    uint16_t adc_full_scale_mv,
    secdb_diode_open_calibration_t *calibration);

secdb_diode_status_t SECDB_Diode_Measure(
    const secdb_diode_open_calibration_t *calibration,
    secdb_diode_result_t *result);

#endif /* SECDB_DIODE_H */
