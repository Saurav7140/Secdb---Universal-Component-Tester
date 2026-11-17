#ifndef SECDB_DIAGNOSTIC_H
#define SECDB_DIAGNOSTIC_H

#include "secdb_bjt.h"
#include "secdb_capacitor.h"
#include "secdb_diode.h"
#include "secdb_resistor.h"
#include <stdbool.h>
#include <stdint.h>

#define SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT  10000UL
#define SECDB_DIAG_MAX_TOLERANCE_BP               5000U

typedef enum {
    SECDB_COMPONENT_NONE = 0,
    SECDB_COMPONENT_RESISTOR,
    SECDB_COMPONENT_CAPACITOR,
    SECDB_COMPONENT_DIODE,
    SECDB_COMPONENT_BJT
} secdb_component_t;

typedef enum {
    SECDB_DIAG_UNIT_NONE = 0,
    SECDB_DIAG_UNIT_OHM,
    SECDB_DIAG_UNIT_NANOFARAD,
    SECDB_DIAG_UNIT_MILLIVOLT,
    SECDB_DIAG_UNIT_HFE_X10
} secdb_diagnostic_unit_t;

typedef enum {
    SECDB_DIAG_SEVERITY_INFORMATION = 0,
    SECDB_DIAG_SEVERITY_PASS,
    SECDB_DIAG_SEVERITY_CAUTION,
    SECDB_DIAG_SEVERITY_FAULT,
    SECDB_DIAG_SEVERITY_SAFETY,
    SECDB_DIAG_SEVERITY_SYSTEM_ERROR
} secdb_diagnostic_severity_t;

typedef enum {
    SECDB_DIAG_CONFIDENCE_OBSERVED = 0,
    SECDB_DIAG_CONFIDENCE_CONDITIONAL,
    SECDB_DIAG_CONFIDENCE_CANDIDATE,
    SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
    SECDB_DIAG_CONFIDENCE_UNAVAILABLE
} secdb_diagnostic_confidence_t;

typedef enum {
    SECDB_DIAG_ACTION_NONE = 0,
    SECDB_DIAG_ACTION_ENTER_REFERENCE,
    SECDB_DIAG_ACTION_CHECK_CONTACTS,
    SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE,
    SECDB_DIAG_ACTION_NOTE_DIODE_ORIENTATION,
    SECDB_DIAG_ACTION_CONFIRM_EMPTY_SOCKET_AND_CALIBRATE,
    SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT,
    SECDB_DIAG_ACTION_COMPARE_WITH_PART_SPECIFICATION,
    SECDB_DIAG_ACTION_REMOVE_POWER_AND_DISCHARGE,
    SECDB_DIAG_ACTION_RUN_ADC_CALIBRATION,
    SECDB_DIAG_ACTION_RETRY_MEASUREMENT,
    SECDB_DIAG_ACTION_SERVICE_FIRMWARE_OR_AFE
} secdb_diagnostic_action_t;

typedef enum {
    SECDB_DIAG_INVALID_ARGUMENT = 0,
    SECDB_DIAG_REFERENCE_INVALID,
    SECDB_DIAG_MEASURED_NO_REFERENCE,
    SECDB_DIAG_VALUE_WITHIN_ENTERED_TOLERANCE,
    SECDB_DIAG_VALUE_OUTSIDE_ENTERED_TOLERANCE,
    SECDB_DIAG_RESISTOR_SHORT_OR_BELOW_RANGE,
    SECDB_DIAG_RESISTOR_OPEN_OR_ABOVE_RANGE,
    SECDB_DIAG_RESISTOR_OUTSIDE_SUPPORTED_RANGE,
    SECDB_DIAG_CAPACITOR_OPEN_OR_BELOW_RANGE,
    SECDB_DIAG_CAPACITOR_ABOVE_RANGE,
    SECDB_DIAG_CAPACITOR_TIMEOUT_AMBIGUOUS,
    SECDB_DIAG_CAPACITOR_INITIAL_DISCHARGE_UNSAFE,
    SECDB_DIAG_CAPACITOR_FINAL_DISCHARGE_UNCONFIRMED,
    SECDB_DIAG_DIODE_OPEN_CALIBRATION_READY,
    SECDB_DIAG_DIODE_CONDUCTING_ANODE_A,
    SECDB_DIAG_DIODE_CONDUCTING_ANODE_B,
    SECDB_DIAG_DIODE_OPEN_OR_NO_CONTACT,
    SECDB_DIAG_DIODE_SHORT,
    SECDB_DIAG_DIODE_LEAKY_BIDIRECTIONAL_OR_UNKNOWN,
    SECDB_DIAG_DIODE_OPEN_CALIBRATION_REQUIRED,
    SECDB_DIAG_DIODE_OPEN_CALIBRATION_FAILED,
    SECDB_DIAG_BJT_ACTIVE_NO_GAIN_REFERENCE,
    SECDB_DIAG_BJT_GAIN_WITHIN_ENTERED_RANGE,
    SECDB_DIAG_BJT_GAIN_OUTSIDE_ENTERED_RANGE,
    SECDB_DIAG_BJT_GAIN_ABOVE_CHARACTERIZED_RANGE,
    SECDB_DIAG_BJT_LOW_GAIN_OR_INADEQUATE_MARGIN,
    SECDB_DIAG_BJT_OPEN_BASE_WRONG_PIN_OR_POLARITY,
    SECDB_DIAG_BJT_OPEN_COLLECTOR_OR_NO_RESPONSE,
    SECDB_DIAG_BJT_BE_SHORT_OR_WRONG_CONNECTION,
    SECDB_DIAG_BJT_BC_SHORT_SATURATION_OR_WRONG_CONNECTION,
    SECDB_DIAG_BJT_SATURATION_OR_INVALID_ACTIVE_REGION,
    SECDB_DIAG_BJT_CE_SHORT_CANDIDATE,
    SECDB_DIAG_BJT_JUNCTION_OUT_OF_RANGE,
    SECDB_DIAG_CALIBRATION_REQUIRED,
    SECDB_DIAG_ADC_FAILURE,
    SECDB_DIAG_AFE_FAILURE,
    SECDB_DIAG_MEASUREMENT_CONFIGURATION_ERROR
} secdb_diagnostic_code_t;

typedef struct {
    bool enabled;
    uint32_t nominal_value;
    uint16_t tolerance_basis_points;
} secdb_value_reference_t;

typedef struct {
    bool enabled;
    uint32_t minimum_hfe_x10;
    uint32_t maximum_hfe_x10;
} secdb_bjt_gain_reference_t;

typedef struct {
    secdb_diagnostic_code_t code;
    secdb_component_t component;
    secdb_diagnostic_severity_t severity;
    secdb_diagnostic_confidence_t confidence;
    secdb_diagnostic_action_t action;
    secdb_diagnostic_unit_t unit;
    uint32_t measured_value;
    bool measured_value_valid;
    bool reference_applied;
    uint32_t reference_low;
    uint32_t reference_high;
    int32_t deviation_basis_points;
} secdb_diagnostic_result_t;

secdb_diagnostic_code_t SECDB_Diagnostic_FromResistor(
    const secdb_resistor_result_t *measurement,
    const secdb_value_reference_t *reference,
    secdb_diagnostic_result_t *diagnostic);

secdb_diagnostic_code_t SECDB_Diagnostic_FromCapacitor(
    const secdb_capacitor_result_t *measurement,
    const secdb_value_reference_t *reference,
    secdb_diagnostic_result_t *diagnostic);

secdb_diagnostic_code_t SECDB_Diagnostic_FromDiode(
    const secdb_diode_result_t *measurement,
    secdb_diagnostic_result_t *diagnostic);

secdb_diagnostic_code_t SECDB_Diagnostic_FromBjt(
    const secdb_bjt_result_t *measurement,
    const secdb_bjt_gain_reference_t *reference,
    secdb_diagnostic_result_t *diagnostic);

const char *SECDB_Diagnostic_CodeText(secdb_diagnostic_code_t code);
const char *SECDB_Diagnostic_ActionText(secdb_diagnostic_action_t action);
const char *SECDB_Diagnostic_UnitText(secdb_diagnostic_unit_t unit);

#endif /* SECDB_DIAGNOSTIC_H */
