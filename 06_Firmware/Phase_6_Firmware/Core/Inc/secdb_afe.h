#ifndef SECDB_AFE_H
#define SECDB_AFE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SECDB_STATUS_OK = 0,
    SECDB_STATUS_INVALID_ARGUMENT,
    SECDB_STATUS_BAD_SEQUENCE,
    SECDB_STATUS_CALIBRATION_REQUIRED
} secdb_status_t;

typedef enum {
    SECDB_AFE_DISABLED = 0,
    SECDB_AFE_CAL_1V1,
    SECDB_AFE_CAL_2V2,
    SECDB_AFE_RESISTOR_10K_TRIAL,
    SECDB_AFE_RESISTOR_1K,
    SECDB_AFE_RESISTOR_100K,
    SECDB_AFE_CAP_DISCHARGE,
    SECDB_AFE_CAP_READY,
    SECDB_AFE_CAP_CHARGE,
    SECDB_AFE_DIODE_AB_READ_A,
    SECDB_AFE_DIODE_AB_READ_B,
    SECDB_AFE_DIODE_BA_READ_A,
    SECDB_AFE_DIODE_BA_READ_B,
    SECDB_AFE_NPN_READ_E,
    SECDB_AFE_NPN_READ_B,
    SECDB_AFE_NPN_READ_C,
    SECDB_AFE_NPN_BASE_OFF_READ_E,
    SECDB_AFE_NPN_BASE_OFF_READ_C,
    SECDB_AFE_PNP_READ_E,
    SECDB_AFE_PNP_READ_B,
    SECDB_AFE_PNP_READ_C,
    SECDB_AFE_PNP_BASE_OFF_READ_E,
    SECDB_AFE_PNP_BASE_OFF_READ_C
} secdb_afe_mode_t;

/* Initializes GPIOs directly into the hardware-safe reset state. */
void SECDB_AFE_Init(void);

/* Disables U_SENSE, disables U_SRC, and tri-states every DUT drive. */
void SECDB_AFE_EnterSafeTransition(void);

/* Phase 6B owns this interlock. It is false after reset and failed calibration. */
void SECDB_AFE_SetMeasurementPermit(bool permitted);
bool SECDB_AFE_IsMeasurementPermitted(void);

/* Applies one complete non-capacitor-charge row of the Phase 5D truth table. */
secdb_status_t SECDB_AFE_ApplyMode(secdb_afe_mode_t mode);

/*
 * CAP_READY must be active first. This function preserves RC sensing,
 * enables the 10 kohm charging path, and captures the time immediately after
 * the U_SRC enable write. No 200 us delay is inserted after the start event.
 */
secdb_status_t SECDB_AFE_StartCapCharge(uint32_t *start_time_us);

secdb_afe_mode_t SECDB_AFE_GetMode(void);

#endif /* SECDB_AFE_H */
