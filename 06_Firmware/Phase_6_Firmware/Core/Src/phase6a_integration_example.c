/*
 * Copy these calls into the CubeMX-generated main.c. This file is an example,
 * not a second application entry point.
 */
#include "main.h"
#include "secdb_afe.h"
#include "secdb_timebase.h"

void SECDB_Phase6A_StartupExample(void)
{
    if (!SECDB_Timebase_Init()) {
        Error_Handler();
    }

    SECDB_AFE_Init();

    /* First legal connected state is a known calibration node, never a DUT. */
    if (SECDB_AFE_ApplyMode(SECDB_AFE_CAL_1V1) != SECDB_STATUS_OK) {
        Error_Handler();
    }
}

void SECDB_Phase6A_CapacitorTimingExample(void)
{
    uint32_t charge_start_us;

    (void)SECDB_AFE_ApplyMode(SECDB_AFE_CAP_DISCHARGE);
    /* Phase 6B will poll the ADC until the discharge threshold or timeout. */
    (void)SECDB_AFE_ApplyMode(SECDB_AFE_CAP_READY);
    (void)SECDB_AFE_StartCapCharge(&charge_start_us);

    /* Phase 6B will use charge_start_us for the threshold-crossing interval. */
    (void)charge_start_us;
    SECDB_AFE_EnterSafeTransition();
}
