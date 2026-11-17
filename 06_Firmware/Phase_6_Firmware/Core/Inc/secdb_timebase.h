#ifndef SECDB_TIMEBASE_H
#define SECDB_TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

bool SECDB_Timebase_Init(void);
uint32_t SECDB_TimeUs(void);
void SECDB_DelayUs(uint32_t delay_us);

#endif /* SECDB_TIMEBASE_H */
