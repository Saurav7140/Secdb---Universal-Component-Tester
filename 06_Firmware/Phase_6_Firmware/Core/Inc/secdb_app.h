#ifndef SECDB_APP_H
#define SECDB_APP_H

#include "secdb_adc.h"
#include "secdb_bjt.h"
#include "secdb_calibration_store.h"
#include "secdb_capacitor.h"
#include "secdb_diagnostic.h"
#include "secdb_diode.h"
#include "secdb_resistor.h"
#include "secdb_st7735.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SECDB_APP_MODE_RESISTOR = 0,
    SECDB_APP_MODE_CAPACITOR,
    SECDB_APP_MODE_DIODE,
    SECDB_APP_MODE_BJT_NPN,
    SECDB_APP_MODE_BJT_PNP,
    SECDB_APP_MODE_COUNT
} secdb_app_mode_t;

typedef enum {
    SECDB_APP_STATE_ADC_CALIBRATION = 0,
    SECDB_APP_STATE_MENU,
    SECDB_APP_STATE_VALUE_REFERENCE,
    SECDB_APP_STATE_TOLERANCE,
    SECDB_APP_STATE_BJT_GAIN_MINIMUM,
    SECDB_APP_STATE_BJT_GAIN_MAXIMUM,
    SECDB_APP_STATE_DIODE_OPEN_CALIBRATION,
    SECDB_APP_STATE_READY,
    SECDB_APP_STATE_MEASURING,
    SECDB_APP_STATE_RESULT,
    SECDB_APP_STATE_SAFETY_LOCK,
    SECDB_APP_STATE_DISPLAY_ERROR
} secdb_app_state_t;

typedef enum {
    SECDB_APP_OK = 0,
    SECDB_APP_INVALID_ARGUMENT,
    SECDB_APP_DISPLAY_NOT_READY,
    SECDB_APP_SAFETY_CONFIRMATION_REQUIRED,
    SECDB_APP_STORAGE_FAILURE
} secdb_app_status_t;

typedef struct {
    secdb_app_state_t state;
    secdb_app_mode_t selected_mode;
    secdb_calibration_image_t calibration;
    secdb_calibration_image_status_t calibration_load_status;
    secdb_adc_calibration_t adc_calibration;
    secdb_diode_open_calibration_t diode_open_calibration;
    secdb_value_reference_t value_reference;
    secdb_bjt_gain_reference_t bjt_gain_reference;
    secdb_resistor_result_t resistor_result;
    secdb_capacitor_result_t capacitor_result;
    secdb_diode_result_t diode_result;
    secdb_bjt_result_t bjt_result;
    secdb_diagnostic_result_t diagnostic;
    uint32_t log_sequence;
    uint8_t reference_value_index;
    uint8_t tolerance_index;
    uint8_t bjt_minimum_index;
    uint8_t bjt_maximum_index;
    secdb_adc_status_t last_adc_calibration_status;
    secdb_st7735_status_t display_status;
    bool last_log_succeeded;
    char ui_lines[7][22];
} secdb_app_t;

secdb_app_status_t SECDB_App_Init(secdb_app_t *app);
void SECDB_App_Tick(secdb_app_t *app);
void SECDB_App_ProcessButtonEvents(secdb_app_t *app, uint32_t events);

secdb_app_status_t SECDB_App_ClearSafetyLock(
    secdb_app_t *app,
    bool user_confirmed_component_removed_and_discharged);

secdb_app_status_t SECDB_App_SaveCalibration(secdb_app_t *app);
secdb_calibration_image_t *SECDB_App_GetCalibrationForService(
    secdb_app_t *app);

#endif /* SECDB_APP_H */
