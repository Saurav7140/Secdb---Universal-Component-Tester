#include "secdb_log.h"
#include <stddef.h>
#include <stdio.h>

#define SECDB_LOG_BUFFER_SIZE  320U

static secdb_log_write_fn writer;

static bool emit(const char *buffer, int length)
{
    if ((writer == NULL) || (buffer == NULL) || (length <= 0) ||
        (length >= (int)SECDB_LOG_BUFFER_SIZE)) {
        return false;
    }
    return writer((const uint8_t *)buffer, (uint16_t)length);
}

void SECDB_Log_Bind(secdb_log_write_fn write_function)
{
    writer = write_function;
}

bool SECDB_Log_WriteHeader(void)
{
    static const char header[] =
        "SECDB,sequence,component,measurement_status,diagnostic_code,"
        "severity,confidence,value,unit,raw_fields\r\n";
    if (writer == NULL) {
        return false;
    }
    return writer((const uint8_t *)header, (uint16_t)(sizeof(header) - 1U));
}

bool SECDB_Log_Resistor(uint32_t sequence,
                        const secdb_resistor_result_t *measurement,
                        const secdb_diagnostic_result_t *diagnostic)
{
    char buffer[SECDB_LOG_BUFFER_SIZE];
    int length;
    if ((measurement == NULL) || (diagnostic == NULL)) {
        return false;
    }
    length = snprintf(buffer, sizeof(buffer),
        "SECDB,%lu,RES,%u,%u,%u,%u,%lu,%s,trial=%u;final=%u;range=%u\r\n",
        (unsigned long)sequence, (unsigned int)measurement->status,
        (unsigned int)diagnostic->code, (unsigned int)diagnostic->severity,
        (unsigned int)diagnostic->confidence,
        (unsigned long)diagnostic->measured_value,
        SECDB_Diagnostic_UnitText(diagnostic->unit),
        (unsigned int)measurement->trial_code,
        (unsigned int)measurement->final_code,
        (unsigned int)measurement->selected_range);
    return emit(buffer, length);
}

bool SECDB_Log_Capacitor(uint32_t sequence,
                         const secdb_capacitor_result_t *measurement,
                         const secdb_diagnostic_result_t *diagnostic)
{
    char buffer[SECDB_LOG_BUFFER_SIZE];
    int length;
    if ((measurement == NULL) || (diagnostic == NULL)) {
        return false;
    }
    length = snprintf(buffer, sizeof(buffer),
        "SECDB,%lu,CAP,%u,%u,%u,%u,%lu,%s,time_us=%lu;last=%u;discharged=%u;measure=%u\r\n",
        (unsigned long)sequence, (unsigned int)measurement->status,
        (unsigned int)diagnostic->code, (unsigned int)diagnostic->severity,
        (unsigned int)diagnostic->confidence,
        (unsigned long)diagnostic->measured_value,
        SECDB_Diagnostic_UnitText(diagnostic->unit),
        (unsigned long)measurement->charge_time_us,
        (unsigned int)measurement->last_raw_code,
        measurement->final_discharge_confirmed ? 1U : 0U,
        (unsigned int)measurement->measurement_status);
    return emit(buffer, length);
}

bool SECDB_Log_Diode(uint32_t sequence,
                     const secdb_diode_result_t *measurement,
                     const secdb_diagnostic_result_t *diagnostic)
{
    char buffer[SECDB_LOG_BUFFER_SIZE];
    int length;
    if ((measurement == NULL) || (diagnostic == NULL)) {
        return false;
    }
    length = snprintf(buffer, sizeof(buffer),
        "SECDB,%lu,DIODE,%u,%u,%u,%u,%lu,%s,ab=%u;ba=%u;type=%u;Aab=%u;Bab=%u;Aba=%u;Bba=%u\r\n",
        (unsigned long)sequence, (unsigned int)measurement->status,
        (unsigned int)diagnostic->code, (unsigned int)diagnostic->severity,
        (unsigned int)diagnostic->confidence,
        (unsigned long)diagnostic->measured_value,
        SECDB_Diagnostic_UnitText(diagnostic->unit),
        (unsigned int)measurement->diff_ab_code,
        (unsigned int)measurement->diff_ba_code,
        (unsigned int)measurement->type,
        (unsigned int)measurement->ab_terminal_a_code,
        (unsigned int)measurement->ab_terminal_b_code,
        (unsigned int)measurement->ba_terminal_a_code,
        (unsigned int)measurement->ba_terminal_b_code);
    return emit(buffer, length);
}

bool SECDB_Log_Bjt(uint32_t sequence,
                   const secdb_bjt_result_t *measurement,
                   const secdb_diagnostic_result_t *diagnostic)
{
    char buffer[SECDB_LOG_BUFFER_SIZE];
    int length;
    if ((measurement == NULL) || (diagnostic == NULL)) {
        return false;
    }
    length = snprintf(buffer, sizeof(buffer),
        "SECDB,%lu,BJT,%u,%u,%u,%u,%lu,%s,pol=%u;E=%u;B=%u;C=%u;J=%u;CE=%u;CB=%u;IBnA=%lu;ICuA=%lu;offCE=%u;offICuA=%lu\r\n",
        (unsigned long)sequence, (unsigned int)measurement->status,
        (unsigned int)diagnostic->code, (unsigned int)diagnostic->severity,
        (unsigned int)diagnostic->confidence,
        (unsigned long)diagnostic->measured_value,
        SECDB_Diagnostic_UnitText(diagnostic->unit),
        (unsigned int)measurement->polarity,
        (unsigned int)measurement->emitter_code,
        (unsigned int)measurement->base_code,
        (unsigned int)measurement->collector_code,
        (unsigned int)measurement->junction_mv,
        (unsigned int)measurement->ce_voltage_mv,
        (unsigned int)measurement->cb_voltage_mv,
        (unsigned long)measurement->base_current_na,
        (unsigned long)measurement->collector_current_ua,
        (unsigned int)measurement->base_off_ce_voltage_mv,
        (unsigned long)measurement->base_off_collector_current_ua);
    return emit(buffer, length);
}
