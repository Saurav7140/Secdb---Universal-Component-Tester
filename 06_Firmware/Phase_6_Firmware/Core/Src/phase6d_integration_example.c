/* Example calls for the CubeMX application; this is not another main(). */
#include "secdb_bjt.h"
#include "secdb_diode.h"

secdb_diode_status_t SECDB_Phase6D_DiodeOpenCalibrationExample(
    uint16_t measured_adc_full_scale_mv,
    secdb_diode_open_calibration_t *calibration)
{
    /* The application must confirm that the diode socket is empty first. */
    return SECDB_Diode_RunOpenCalibration(
        measured_adc_full_scale_mv, calibration);
}

secdb_diode_status_t SECDB_Phase6D_DiodeExample(
    const secdb_diode_open_calibration_t *calibration,
    secdb_diode_result_t *result)
{
    return SECDB_Diode_Measure(calibration, result);
}

secdb_bjt_status_t SECDB_Phase6D_BjtExample(
    secdb_bjt_polarity_t user_selected_polarity,
    secdb_bjt_result_t *result)
{
    const secdb_bjt_calibration_t path_calibration =
        SECDB_BJT_DefaultCalibration();

    /* The UI must first ask for NPN/PNP and require known E/B/C insertion. */
    return SECDB_BJT_Measure(
        user_selected_polarity, &path_calibration, result);
}
