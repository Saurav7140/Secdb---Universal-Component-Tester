#ifndef SECDB_BJT_H
#define SECDB_BJT_H

#include "secdb_adc.h"
#include "secdb_afe.h"
#include <stdbool.h>
#include <stdint.h>

#define SECDB_BJT_JUNCTION_MIN_MV             450U
#define SECDB_BJT_JUNCTION_MAX_MV             850U
#define SECDB_BJT_BE_SHORT_MAX_MV             100U
#define SECDB_BJT_ACTIVE_CE_MIN_MV             300U
#define SECDB_BJT_ACTIVE_CB_MIN_MV             200U
#define SECDB_BJT_COLLECTOR_MARGIN_MIN_MV       40U
#define SECDB_BJT_BASE_DRIVE_MARGIN_MIN_MV     1000U
#define SECDB_BJT_HFE_MIN_X10                  200U
#define SECDB_BJT_HFE_MAX_X10                 5000U
#define SECDB_BJT_BASE_OFF_CE_SHORT_MAX_MV     100U
#define SECDB_BJT_BASE_OFF_SHORT_MIN_UA        500U

typedef enum {
    SECDB_BJT_NPN = 0,
    SECDB_BJT_PNP
} secdb_bjt_polarity_t;

typedef enum {
    SECDB_BJT_ACTIVE_INDICATIVE_HFE = 0,
    SECDB_BJT_ACTIVE_GAIN_ABOVE_CHARACTERIZED_RANGE,
    SECDB_BJT_LOW_GAIN_OR_INADEQUATE_MARGIN,
    SECDB_BJT_OPEN_BASE_WRONG_PIN_OR_POLARITY,
    SECDB_BJT_OPEN_COLLECTOR_OR_NO_RESPONSE,
    SECDB_BJT_BE_SHORT_OR_WRONG_CONNECTION,
    SECDB_BJT_BC_SHORT_SATURATION_OR_WRONG_CONNECTION,
    SECDB_BJT_SATURATION_OR_INVALID_ACTIVE_REGION,
    SECDB_BJT_CE_SHORT_CANDIDATE,
    SECDB_BJT_JUNCTION_OUT_OF_RANGE,
    SECDB_BJT_CALIBRATION_REQUIRED,
    SECDB_BJT_INVALID_ARGUMENT,
    SECDB_BJT_AFE_FAILURE,
    SECDB_BJT_ADC_FAILURE
} secdb_bjt_status_t;

typedef struct {
    uint16_t drive_low_mv;
    uint16_t drive_high_mv;
    uint16_t adc_full_scale_mv;
    uint32_t base_path_ohm;
    uint32_t collector_path_ohm;
} secdb_bjt_calibration_t;

typedef struct {
    secdb_bjt_status_t status;
    secdb_bjt_polarity_t polarity;
    uint16_t emitter_code;
    uint16_t base_code;
    uint16_t collector_code;
    uint16_t emitter_mv;
    uint16_t base_mv;
    uint16_t collector_mv;
    uint16_t junction_mv;
    uint16_t ce_voltage_mv;
    uint16_t cb_voltage_mv;
    uint32_t base_current_na;
    uint32_t collector_current_ua;
    uint32_t indicative_hfe_x10;
    bool hfe_valid;
    bool base_off_performed;
    uint16_t base_off_emitter_mv;
    uint16_t base_off_collector_mv;
    uint16_t base_off_ce_voltage_mv;
    uint32_t base_off_collector_current_ua;
    secdb_adc_status_t adc_status;
    secdb_status_t afe_status;
} secdb_bjt_result_t;

/* Phase 5C simulation constants; replace with measured board values in Phase 7. */
secdb_bjt_calibration_t SECDB_BJT_DefaultCalibration(void);

secdb_bjt_status_t SECDB_BJT_Measure(
    secdb_bjt_polarity_t polarity,
    const secdb_bjt_calibration_t *calibration,
    secdb_bjt_result_t *result);

#endif /* SECDB_BJT_H */
