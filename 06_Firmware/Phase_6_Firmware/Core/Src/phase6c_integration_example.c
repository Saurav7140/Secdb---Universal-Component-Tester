/* Example calls for the CubeMX application; this is not another main(). */
#include "secdb_capacitor.h"
#include "secdb_resistor.h"

secdb_resistor_status_t SECDB_Phase6C_ResistorExample(
    secdb_resistor_result_t *result)
{
    const secdb_resistor_calibration_t path_calibration =
        SECDB_Resistor_DefaultCalibration();

    return SECDB_Resistor_Measure(&path_calibration, result);
}

secdb_capacitor_status_t SECDB_Phase6C_CapacitorExample(
    secdb_capacitor_result_t *result)
{
    return SECDB_Capacitor_Measure(
        SECDB_Capacitor_DefaultTimingDenominatorMilliohm(), result);
}
