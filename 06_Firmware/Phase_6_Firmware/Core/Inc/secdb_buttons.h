#ifndef SECDB_BUTTONS_H
#define SECDB_BUTTONS_H

#include <stdint.h>

#define SECDB_BUTTON_DEBOUNCE_MS       25UL
#define SECDB_BUTTON_REPEAT_DELAY_MS  600UL
#define SECDB_BUTTON_REPEAT_RATE_MS   150UL

typedef enum {
    SECDB_BUTTON_EVENT_NONE       = 0U,
    SECDB_BUTTON_EVENT_MODE_PRESS = 1U << 0,
    SECDB_BUTTON_EVENT_UP_PRESS   = 1U << 1,
    SECDB_BUTTON_EVENT_DOWN_PRESS = 1U << 2,
    SECDB_BUTTON_EVENT_UP_REPEAT  = 1U << 3,
    SECDB_BUTTON_EVENT_DOWN_REPEAT = 1U << 4
} secdb_button_event_t;

void SECDB_Buttons_Init(void);
uint32_t SECDB_Buttons_Poll(void);

#endif /* SECDB_BUTTONS_H */
