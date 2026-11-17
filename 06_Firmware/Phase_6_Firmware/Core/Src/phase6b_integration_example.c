/* Example calls for the CubeMX-generated main.c; this is not another main(). */
#include "main.h"
#include "secdb_adc.h"
#include "secdb_afe.h"

extern ADC_HandleTypeDef hadc1;

void SECDB_Phase6B_StartupExample(void)
{
    secdb_adc_calibration_t calibration_result;

    if (SECDB_ADC_Bind(&hadc1) != SECDB_ADC_OK) {
        Error_Handler();
    }
    if (SECDB_ADC_RunCalibration(&calibration_result) != SECDB_ADC_OK) {
        /* Keep component tests locked; display a calibration fault later. */
        SECDB_AFE_EnterSafeTransition();
        Error_Handler();
    }
}

secdb_adc_status_t SECDB_Phase6B_ReadExample(uint16_t *corrected_code)
{
    secdb_adc_reading_t reading;

    if (SECDB_AFE_ApplyMode(SECDB_AFE_RESISTOR_10K_TRIAL) != SECDB_STATUS_OK) {
        return SECDB_ADC_CALIBRATION_REQUIRED;
    }

    return SECDB_ADC_ReadCorrectedAveraged(
        SECDB_ADC_DEFAULT_SAMPLE_COUNT,
        SECDB_ADC_DEFAULT_TIMEOUT_MS,
        &reading,
        corrected_code);
}
