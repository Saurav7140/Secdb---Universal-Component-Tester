/* Result-model examples for the later Phase 6F UI; this is not a main(). */
#include "secdb_diagnostic.h"

secdb_diagnostic_code_t SECDB_Phase6E_ResistorExample(
    const secdb_resistor_result_t *measurement,
    uint32_t entered_nominal_ohm,
    uint16_t entered_tolerance_basis_points,
    secdb_diagnostic_result_t *diagnostic)
{
    const secdb_value_reference_t reference = {
        true, entered_nominal_ohm, entered_tolerance_basis_points
    };
    return SECDB_Diagnostic_FromResistor(
        measurement, &reference, diagnostic);
}

secdb_diagnostic_code_t SECDB_Phase6E_CapacitorNoReferenceExample(
    const secdb_capacitor_result_t *measurement,
    secdb_diagnostic_result_t *diagnostic)
{
    return SECDB_Diagnostic_FromCapacitor(
        measurement, NULL, diagnostic);
}

secdb_diagnostic_code_t SECDB_Phase6E_DiodeExample(
    const secdb_diode_result_t *measurement,
    secdb_diagnostic_result_t *diagnostic)
{
    return SECDB_Diagnostic_FromDiode(measurement, diagnostic);
}

secdb_diagnostic_code_t SECDB_Phase6E_BjtExample(
    const secdb_bjt_result_t *measurement,
    uint32_t expected_minimum_hfe_x10,
    uint32_t expected_maximum_hfe_x10,
    secdb_diagnostic_result_t *diagnostic)
{
    const secdb_bjt_gain_reference_t reference = {
        true, expected_minimum_hfe_x10, expected_maximum_hfe_x10
    };
    return SECDB_Diagnostic_FromBjt(measurement, &reference, diagnostic);
}
