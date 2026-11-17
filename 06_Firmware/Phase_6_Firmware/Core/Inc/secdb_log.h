#ifndef SECDB_LOG_H
#define SECDB_LOG_H

#include "secdb_bjt.h"
#include "secdb_capacitor.h"
#include "secdb_diagnostic.h"
#include "secdb_diode.h"
#include "secdb_resistor.h"
#include <stdbool.h>
#include <stdint.h>

typedef bool (*secdb_log_write_fn)(const uint8_t *data, uint16_t length);

void SECDB_Log_Bind(secdb_log_write_fn write_function);
bool SECDB_Log_WriteHeader(void);
bool SECDB_Log_Resistor(uint32_t sequence,
                        const secdb_resistor_result_t *measurement,
                        const secdb_diagnostic_result_t *diagnostic);
bool SECDB_Log_Capacitor(uint32_t sequence,
                         const secdb_capacitor_result_t *measurement,
                         const secdb_diagnostic_result_t *diagnostic);
bool SECDB_Log_Diode(uint32_t sequence,
                     const secdb_diode_result_t *measurement,
                     const secdb_diagnostic_result_t *diagnostic);
bool SECDB_Log_Bjt(uint32_t sequence,
                   const secdb_bjt_result_t *measurement,
                   const secdb_diagnostic_result_t *diagnostic);

#endif /* SECDB_LOG_H */
