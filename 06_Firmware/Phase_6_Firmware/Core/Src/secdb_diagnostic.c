#include "secdb_diagnostic.h"
#include <limits.h>
#include <stddef.h>

static void initialize_diagnostic(secdb_component_t component,
                                  secdb_diagnostic_result_t *diagnostic)
{
    diagnostic->code = SECDB_DIAG_INVALID_ARGUMENT;
    diagnostic->component = component;
    diagnostic->severity = SECDB_DIAG_SEVERITY_SYSTEM_ERROR;
    diagnostic->confidence = SECDB_DIAG_CONFIDENCE_UNAVAILABLE;
    diagnostic->action = SECDB_DIAG_ACTION_SERVICE_FIRMWARE_OR_AFE;
    diagnostic->unit = SECDB_DIAG_UNIT_NONE;
    diagnostic->measured_value = 0UL;
    diagnostic->measured_value_valid = false;
    diagnostic->reference_applied = false;
    diagnostic->reference_low = 0UL;
    diagnostic->reference_high = 0UL;
    diagnostic->deviation_basis_points = 0L;
}

static secdb_diagnostic_code_t set_diagnostic(
    secdb_diagnostic_result_t *diagnostic,
    secdb_diagnostic_code_t code,
    secdb_diagnostic_severity_t severity,
    secdb_diagnostic_confidence_t confidence,
    secdb_diagnostic_action_t action)
{
    diagnostic->code = code;
    diagnostic->severity = severity;
    diagnostic->confidence = confidence;
    diagnostic->action = action;
    return code;
}

static bool reference_is_valid(const secdb_value_reference_t *reference,
                               uint32_t minimum,
                               uint32_t maximum)
{
    return (reference != NULL) && reference->enabled &&
           (reference->nominal_value >= minimum) &&
           (reference->nominal_value <= maximum) &&
           (reference->tolerance_basis_points <=
            SECDB_DIAG_MAX_TOLERANCE_BP);
}

static uint32_t rounded_ratio(uint64_t numerator, uint32_t denominator)
{
    return (uint32_t)((numerator + (denominator / 2UL)) / denominator);
}

static int32_t calculate_deviation_basis_points(uint32_t measured,
                                                uint32_t nominal)
{
    int64_t difference = (int64_t)measured - (int64_t)nominal;
    int64_t scaled = difference *
                     (int64_t)SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT;
    int64_t magnitude;

    if (scaled >= 0) {
        scaled = (scaled + ((int64_t)nominal / 2LL)) / (int64_t)nominal;
    } else {
        magnitude = -scaled;
        scaled = -((magnitude + ((int64_t)nominal / 2LL)) /
                   (int64_t)nominal);
    }
    if (scaled > INT32_MAX) {
        return INT32_MAX;
    }
    if (scaled < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)scaled;
}

static secdb_diagnostic_code_t apply_value_reference(
    uint32_t measured,
    secdb_diagnostic_unit_t unit,
    const secdb_value_reference_t *reference,
    uint32_t supported_minimum,
    uint32_t supported_maximum,
    secdb_diagnostic_result_t *diagnostic)
{
    uint64_t absolute_difference;
    uint64_t allowed_difference;

    diagnostic->unit = unit;
    diagnostic->measured_value = measured;
    diagnostic->measured_value_valid = true;

    if ((reference == NULL) || !reference->enabled) {
        return set_diagnostic(
            diagnostic, SECDB_DIAG_MEASURED_NO_REFERENCE,
            SECDB_DIAG_SEVERITY_INFORMATION,
            SECDB_DIAG_CONFIDENCE_OBSERVED,
            SECDB_DIAG_ACTION_ENTER_REFERENCE);
    }
    if (!reference_is_valid(reference, supported_minimum, supported_maximum)) {
        return set_diagnostic(
            diagnostic, SECDB_DIAG_REFERENCE_INVALID,
            SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
            SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
            SECDB_DIAG_ACTION_ENTER_REFERENCE);
    }

    diagnostic->reference_applied = true;
    diagnostic->reference_low = rounded_ratio(
        (uint64_t)reference->nominal_value *
        (SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT -
         reference->tolerance_basis_points),
        SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT);
    diagnostic->reference_high = rounded_ratio(
        (uint64_t)reference->nominal_value *
        (SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT +
         reference->tolerance_basis_points),
        SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT);
    diagnostic->deviation_basis_points =
        calculate_deviation_basis_points(measured, reference->nominal_value);

    absolute_difference = (measured >= reference->nominal_value)
        ? (uint64_t)measured - reference->nominal_value
        : (uint64_t)reference->nominal_value - measured;
    allowed_difference = (uint64_t)reference->nominal_value *
                         reference->tolerance_basis_points;

    if ((absolute_difference *
         SECDB_DIAG_BASIS_POINTS_PER_100_PERCENT) <= allowed_difference) {
        return set_diagnostic(
            diagnostic, SECDB_DIAG_VALUE_WITHIN_ENTERED_TOLERANCE,
            SECDB_DIAG_SEVERITY_PASS,
            SECDB_DIAG_CONFIDENCE_CONDITIONAL,
            SECDB_DIAG_ACTION_NONE);
    }
    return set_diagnostic(
        diagnostic, SECDB_DIAG_VALUE_OUTSIDE_ENTERED_TOLERANCE,
        SECDB_DIAG_SEVERITY_CAUTION,
        SECDB_DIAG_CONFIDENCE_CANDIDATE,
        SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
}

static secdb_diagnostic_code_t map_common_failure(
    bool calibration_required,
    bool adc_failure,
    bool afe_failure,
    secdb_diagnostic_result_t *diagnostic)
{
    if (calibration_required) {
        return set_diagnostic(
            diagnostic, SECDB_DIAG_CALIBRATION_REQUIRED,
            SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
            SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
            SECDB_DIAG_ACTION_RUN_ADC_CALIBRATION);
    }
    if (adc_failure) {
        return set_diagnostic(
            diagnostic, SECDB_DIAG_ADC_FAILURE,
            SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
            SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
            SECDB_DIAG_ACTION_RETRY_MEASUREMENT);
    }
    if (afe_failure) {
        return set_diagnostic(
            diagnostic, SECDB_DIAG_AFE_FAILURE,
            SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
            SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
            SECDB_DIAG_ACTION_SERVICE_FIRMWARE_OR_AFE);
    }
    return set_diagnostic(
        diagnostic, SECDB_DIAG_MEASUREMENT_CONFIGURATION_ERROR,
        SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
        SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
        SECDB_DIAG_ACTION_SERVICE_FIRMWARE_OR_AFE);
}

secdb_diagnostic_code_t SECDB_Diagnostic_FromResistor(
    const secdb_resistor_result_t *measurement,
    const secdb_value_reference_t *reference,
    secdb_diagnostic_result_t *diagnostic)
{
    if (diagnostic == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }
    initialize_diagnostic(SECDB_COMPONENT_RESISTOR, diagnostic);
    if (measurement == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }

    switch (measurement->status) {
    case SECDB_RESISTOR_OK:
        return apply_value_reference(
            measurement->resistance_ohm, SECDB_DIAG_UNIT_OHM, reference,
            SECDB_RESISTOR_MIN_SUPPORTED_OHM,
            SECDB_RESISTOR_MAX_SUPPORTED_OHM, diagnostic);
    case SECDB_RESISTOR_SHORT_OR_BELOW_MEASURABLE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_RESISTOR_SHORT_OR_BELOW_RANGE,
            SECDB_DIAG_SEVERITY_FAULT, SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_CHECK_CONTACTS);
    case SECDB_RESISTOR_OPEN_OR_ABOVE_MEASURABLE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_RESISTOR_OPEN_OR_ABOVE_RANGE,
            SECDB_DIAG_SEVERITY_FAULT, SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_CHECK_CONTACTS);
    case SECDB_RESISTOR_BELOW_SUPPORTED_RANGE:
    case SECDB_RESISTOR_ABOVE_SUPPORTED_RANGE:
        diagnostic->unit = SECDB_DIAG_UNIT_OHM;
        diagnostic->measured_value = measurement->resistance_ohm;
        diagnostic->measured_value_valid = true;
        return set_diagnostic(
            diagnostic, SECDB_DIAG_RESISTOR_OUTSIDE_SUPPORTED_RANGE,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_CANDIDATE,
            SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
    case SECDB_RESISTOR_INVALID_ARGUMENT:
        return map_common_failure(false, false, false, diagnostic);
    default:
        return map_common_failure(
            measurement->status == SECDB_RESISTOR_CALIBRATION_REQUIRED,
            measurement->status == SECDB_RESISTOR_ADC_FAILURE,
            measurement->status == SECDB_RESISTOR_AFE_FAILURE,
            diagnostic);
    }
}

secdb_diagnostic_code_t SECDB_Diagnostic_FromCapacitor(
    const secdb_capacitor_result_t *measurement,
    const secdb_value_reference_t *reference,
    secdb_diagnostic_result_t *diagnostic)
{
    if (diagnostic == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }
    initialize_diagnostic(SECDB_COMPONENT_CAPACITOR, diagnostic);
    if (measurement == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }

    switch (measurement->status) {
    case SECDB_CAP_OK:
        return apply_value_reference(
            measurement->capacitance_nf, SECDB_DIAG_UNIT_NANOFARAD,
            reference, SECDB_CAP_MIN_SUPPORTED_NF,
            SECDB_CAP_MAX_SUPPORTED_NF, diagnostic);
    case SECDB_CAP_OPEN_OR_BELOW_100NF:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_CAPACITOR_OPEN_OR_BELOW_RANGE,
            SECDB_DIAG_SEVERITY_FAULT, SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_CHECK_CONTACTS);
    case SECDB_CAP_ABOVE_100UF:
        diagnostic->unit = SECDB_DIAG_UNIT_NANOFARAD;
        diagnostic->measured_value = measurement->capacitance_nf;
        diagnostic->measured_value_valid = measurement->capacitance_nf != 0UL;
        return set_diagnostic(
            diagnostic, SECDB_DIAG_CAPACITOR_ABOVE_RANGE,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_CANDIDATE,
            SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
    case SECDB_CAP_TIMEOUT_SHORT_LEAKAGE_OR_ABOVE_RANGE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_CAPACITOR_TIMEOUT_AMBIGUOUS,
            SECDB_DIAG_SEVERITY_FAULT, SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
    case SECDB_CAP_INITIAL_DISCHARGE_FAILED_OR_EXTERNAL_VOLTAGE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_CAPACITOR_INITIAL_DISCHARGE_UNSAFE,
            SECDB_DIAG_SEVERITY_SAFETY,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_REMOVE_POWER_AND_DISCHARGE);
    case SECDB_CAP_FINAL_DISCHARGE_FAILED:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_CAPACITOR_FINAL_DISCHARGE_UNCONFIRMED,
            SECDB_DIAG_SEVERITY_SAFETY,
            SECDB_DIAG_CONFIDENCE_OBSERVED,
            SECDB_DIAG_ACTION_REMOVE_POWER_AND_DISCHARGE);
    case SECDB_CAP_INVALID_ARGUMENT:
        return map_common_failure(false, false, false, diagnostic);
    default:
        return map_common_failure(
            measurement->status == SECDB_CAP_CALIBRATION_REQUIRED,
            measurement->status == SECDB_CAP_ADC_FAILURE,
            measurement->status == SECDB_CAP_AFE_FAILURE,
            diagnostic);
    }
}

secdb_diagnostic_code_t SECDB_Diagnostic_FromDiode(
    const secdb_diode_result_t *measurement,
    secdb_diagnostic_result_t *diagnostic)
{
    if (diagnostic == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }
    initialize_diagnostic(SECDB_COMPONENT_DIODE, diagnostic);
    if (measurement == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }

    if ((measurement->status == SECDB_DIODE_OK_ANODE_A) ||
        (measurement->status == SECDB_DIODE_OK_ANODE_B)) {
        diagnostic->unit = SECDB_DIAG_UNIT_MILLIVOLT;
        diagnostic->measured_value = measurement->forward_mv;
        diagnostic->measured_value_valid = true;
        return set_diagnostic(
            diagnostic,
            (measurement->status == SECDB_DIODE_OK_ANODE_A)
                ? SECDB_DIAG_DIODE_CONDUCTING_ANODE_A
                : SECDB_DIAG_DIODE_CONDUCTING_ANODE_B,
            SECDB_DIAG_SEVERITY_PASS,
            (measurement->type == SECDB_DIODE_TYPE_UNCERTAIN)
                ? SECDB_DIAG_CONFIDENCE_CONDITIONAL
                : SECDB_DIAG_CONFIDENCE_OBSERVED,
            SECDB_DIAG_ACTION_NOTE_DIODE_ORIENTATION);
    }

    switch (measurement->status) {
    case SECDB_DIODE_CALIBRATION_READY:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_DIODE_OPEN_CALIBRATION_READY,
            SECDB_DIAG_SEVERITY_INFORMATION,
            SECDB_DIAG_CONFIDENCE_OBSERVED, SECDB_DIAG_ACTION_NONE);
    case SECDB_DIODE_OPEN_OR_NO_CONTACT:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_DIODE_OPEN_OR_NO_CONTACT,
            SECDB_DIAG_SEVERITY_FAULT, SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_CHECK_CONTACTS);
    case SECDB_DIODE_SHORT:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_DIODE_SHORT,
            SECDB_DIAG_SEVERITY_FAULT, SECDB_DIAG_CONFIDENCE_OBSERVED,
            SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
    case SECDB_DIODE_LEAKY_BIDIRECTIONAL_OR_UNKNOWN:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_DIODE_LEAKY_BIDIRECTIONAL_OR_UNKNOWN,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
    case SECDB_DIODE_OPEN_CALIBRATION_REQUIRED:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_DIODE_OPEN_CALIBRATION_REQUIRED,
            SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
            SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
            SECDB_DIAG_ACTION_CONFIRM_EMPTY_SOCKET_AND_CALIBRATE);
    case SECDB_DIODE_OPEN_CALIBRATION_FAILED:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_DIODE_OPEN_CALIBRATION_FAILED,
            SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
            SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
            SECDB_DIAG_ACTION_CONFIRM_EMPTY_SOCKET_AND_CALIBRATE);
    case SECDB_DIODE_INVALID_ARGUMENT:
        return map_common_failure(false, false, false, diagnostic);
    default:
        return map_common_failure(
            measurement->status == SECDB_DIODE_ADC_CALIBRATION_REQUIRED,
            measurement->status == SECDB_DIODE_ADC_FAILURE,
            measurement->status == SECDB_DIODE_AFE_FAILURE,
            diagnostic);
    }
}

secdb_diagnostic_code_t SECDB_Diagnostic_FromBjt(
    const secdb_bjt_result_t *measurement,
    const secdb_bjt_gain_reference_t *reference,
    secdb_diagnostic_result_t *diagnostic)
{
    if (diagnostic == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }
    initialize_diagnostic(SECDB_COMPONENT_BJT, diagnostic);
    if (measurement == NULL) {
        return SECDB_DIAG_INVALID_ARGUMENT;
    }

    diagnostic->unit = SECDB_DIAG_UNIT_HFE_X10;
    diagnostic->measured_value = measurement->indicative_hfe_x10;
    diagnostic->measured_value_valid = measurement->hfe_valid;

    if (measurement->status == SECDB_BJT_ACTIVE_INDICATIVE_HFE) {
        if ((reference == NULL) || !reference->enabled) {
            return set_diagnostic(
                diagnostic, SECDB_DIAG_BJT_ACTIVE_NO_GAIN_REFERENCE,
                SECDB_DIAG_SEVERITY_PASS,
                SECDB_DIAG_CONFIDENCE_CONDITIONAL,
                SECDB_DIAG_ACTION_COMPARE_WITH_PART_SPECIFICATION);
        }
        if ((reference->minimum_hfe_x10 < SECDB_BJT_HFE_MIN_X10) ||
            (reference->maximum_hfe_x10 > SECDB_BJT_HFE_MAX_X10) ||
            (reference->minimum_hfe_x10 > reference->maximum_hfe_x10)) {
            return set_diagnostic(
                diagnostic, SECDB_DIAG_REFERENCE_INVALID,
                SECDB_DIAG_SEVERITY_SYSTEM_ERROR,
                SECDB_DIAG_CONFIDENCE_UNAVAILABLE,
                SECDB_DIAG_ACTION_COMPARE_WITH_PART_SPECIFICATION);
        }
        diagnostic->reference_applied = true;
        diagnostic->reference_low = reference->minimum_hfe_x10;
        diagnostic->reference_high = reference->maximum_hfe_x10;
        if ((measurement->indicative_hfe_x10 >= reference->minimum_hfe_x10) &&
            (measurement->indicative_hfe_x10 <= reference->maximum_hfe_x10)) {
            return set_diagnostic(
                diagnostic, SECDB_DIAG_BJT_GAIN_WITHIN_ENTERED_RANGE,
                SECDB_DIAG_SEVERITY_PASS,
                SECDB_DIAG_CONFIDENCE_CONDITIONAL,
                SECDB_DIAG_ACTION_NONE);
        }
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_GAIN_OUTSIDE_ENTERED_RANGE,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_CANDIDATE,
            SECDB_DIAG_ACTION_COMPARE_WITH_PART_SPECIFICATION);
    }

    diagnostic->measured_value_valid = false;
    switch (measurement->status) {
    case SECDB_BJT_ACTIVE_GAIN_ABOVE_CHARACTERIZED_RANGE:
        diagnostic->measured_value = measurement->indicative_hfe_x10;
        diagnostic->measured_value_valid = true;
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_GAIN_ABOVE_CHARACTERIZED_RANGE,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_CANDIDATE,
            SECDB_DIAG_ACTION_COMPARE_WITH_PART_SPECIFICATION);
    case SECDB_BJT_LOW_GAIN_OR_INADEQUATE_MARGIN:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_LOW_GAIN_OR_INADEQUATE_MARGIN,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_OPEN_BASE_WRONG_PIN_OR_POLARITY:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_OPEN_BASE_WRONG_PIN_OR_POLARITY,
            SECDB_DIAG_SEVERITY_FAULT,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_OPEN_COLLECTOR_OR_NO_RESPONSE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_OPEN_COLLECTOR_OR_NO_RESPONSE,
            SECDB_DIAG_SEVERITY_FAULT,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_BE_SHORT_OR_WRONG_CONNECTION:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_BE_SHORT_OR_WRONG_CONNECTION,
            SECDB_DIAG_SEVERITY_FAULT,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_BC_SHORT_SATURATION_OR_WRONG_CONNECTION:
        return set_diagnostic(
            diagnostic,
            SECDB_DIAG_BJT_BC_SHORT_SATURATION_OR_WRONG_CONNECTION,
            SECDB_DIAG_SEVERITY_FAULT,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_SATURATION_OR_INVALID_ACTIVE_REGION:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_SATURATION_OR_INVALID_ACTIVE_REGION,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_CE_SHORT_CANDIDATE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_CE_SHORT_CANDIDATE,
            SECDB_DIAG_SEVERITY_FAULT,
            SECDB_DIAG_CONFIDENCE_CANDIDATE,
            SECDB_DIAG_ACTION_CHECK_VALUE_OR_REPLACE);
    case SECDB_BJT_JUNCTION_OUT_OF_RANGE:
        return set_diagnostic(
            diagnostic, SECDB_DIAG_BJT_JUNCTION_OUT_OF_RANGE,
            SECDB_DIAG_SEVERITY_CAUTION,
            SECDB_DIAG_CONFIDENCE_AMBIGUOUS,
            SECDB_DIAG_ACTION_VERIFY_BJT_TYPE_AND_PINOUT);
    case SECDB_BJT_INVALID_ARGUMENT:
        return map_common_failure(false, false, false, diagnostic);
    default:
        return map_common_failure(
            measurement->status == SECDB_BJT_CALIBRATION_REQUIRED,
            measurement->status == SECDB_BJT_ADC_FAILURE,
            measurement->status == SECDB_BJT_AFE_FAILURE,
            diagnostic);
    }
}

const char *SECDB_Diagnostic_CodeText(secdb_diagnostic_code_t code)
{
    static const char *const text[] = {
        "Invalid diagnostic request",
        "Entered reference is invalid",
        "Measurement complete; no reference entered",
        "Within entered tolerance",
        "Outside entered tolerance; wrong value or degraded candidate",
        "Short or resistance below measurable range",
        "Open contact or resistance above measurable range",
        "Resistance outside supported accuracy range",
        "Open contact or capacitance below 100 nF",
        "Capacitance above 100 uF range",
        "Charge timeout: short, leakage, or above range",
        "Initial discharge failed or external voltage detected",
        "Final discharge could not be confirmed",
        "Empty-socket diode calibration ready",
        "Diode conducts A to B; anode is A",
        "Diode conducts B to A; anode is B",
        "Diode open or no contact",
        "Diode shorted",
        "Bidirectional conduction: leaky or unknown device",
        "Empty-socket diode calibration required",
        "Empty-socket diode calibration failed",
        "BJT active; indicative gain has no entered reference",
        "BJT gain within entered range",
        "BJT gain outside entered range",
        "BJT gain above characterized tester range",
        "Low gain or inadequate active-region margin",
        "Open base, wrong pins, or wrong polarity",
        "Open collector or no collector response",
        "B-E short or wrong connection",
        "B-C short, saturation, or wrong connection",
        "Saturation or invalid active region",
        "C-E short candidate after base-off check",
        "Base-emitter junction outside expected range",
        "ADC calibration required",
        "ADC measurement failed",
        "Analog front-end operation failed",
        "Measurement configuration error"
    };
    const uint32_t index = (uint32_t)code;
    if (index >= (sizeof(text) / sizeof(text[0]))) {
        return "Unknown diagnostic code";
    }
    return text[index];
}

const char *SECDB_Diagnostic_ActionText(secdb_diagnostic_action_t action)
{
    static const char *const text[] = {
        "No action required",
        "Enter nominal value and tolerance for comparison",
        "Check socket contact and retest",
        "Verify component value with a reference instrument",
        "Note the indicated anode orientation",
        "Remove the diode; confirm empty socket and recalibrate",
        "Verify NPN/PNP selection and E-B-C pin order",
        "Compare with the part specification or a known-good device",
        "Remove power and discharge the component safely",
        "Run ADC calibration before testing",
        "Retry the measurement",
        "Check firmware configuration and analog front end"
    };
    const uint32_t index = (uint32_t)action;
    if (index >= (sizeof(text) / sizeof(text[0]))) {
        return "Unknown action";
    }
    return text[index];
}

const char *SECDB_Diagnostic_UnitText(secdb_diagnostic_unit_t unit)
{
    static const char *const text[] = {"", "ohm", "nF", "mV", "hFE x10"};
    const uint32_t index = (uint32_t)unit;
    if (index >= (sizeof(text) / sizeof(text[0]))) {
        return "";
    }
    return text[index];
}
