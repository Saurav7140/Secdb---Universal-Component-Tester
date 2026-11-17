#include "secdb_timebase.h"
#include "stm32f4xx_hal.h"

static uint32_t cycles_per_us;
static uint32_t last_cycle_count;
static uint32_t accumulated_us;
static uint32_t remainder_cycles;

bool SECDB_Timebase_Init(void)
{
    cycles_per_us = SystemCoreClock / 1000000UL;
    if (cycles_per_us == 0UL) {
        return false;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0UL;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    last_cycle_count = 0UL;
    accumulated_us = 0UL;
    remainder_cycles = 0UL;

    return (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0UL;
}

uint32_t SECDB_TimeUs(void)
{
    uint32_t primask;
    uint32_t now;
    uint32_t elapsed_cycles;
    uint64_t total_cycles;
    uint32_t result;

    if (cycles_per_us == 0UL) {
        return 0UL;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    now = DWT->CYCCNT;
    elapsed_cycles = now - last_cycle_count;
    last_cycle_count = now;

    total_cycles = (uint64_t)remainder_cycles + elapsed_cycles;
    accumulated_us += (uint32_t)(total_cycles / cycles_per_us);
    remainder_cycles = (uint32_t)(total_cycles % cycles_per_us);
    result = accumulated_us;

    if (primask == 0UL) {
        __enable_irq();
    }
    return result;
}

void SECDB_DelayUs(uint32_t delay_us)
{
    const uint32_t start = DWT->CYCCNT;
    const uint32_t delay_cycles = delay_us * cycles_per_us;

    if (cycles_per_us == 0UL) {
        return;
    }

    while ((uint32_t)(DWT->CYCCNT - start) < delay_cycles) {
        __NOP();
    }
}
