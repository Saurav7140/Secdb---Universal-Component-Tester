#include "secdb_buttons.h"
#include "secdb_board.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t press_event;
    uint32_t repeat_event;
    bool raw_pressed;
    bool stable_pressed;
    uint32_t raw_change_ms;
    uint32_t press_start_ms;
    uint32_t last_repeat_ms;
} button_state_t;

static button_state_t buttons[3];

static bool pin_is_pressed(const button_state_t *button)
{
    return HAL_GPIO_ReadPin(button->port, button->pin) == GPIO_PIN_RESET;
}

static uint32_t poll_one(button_state_t *button, uint32_t now_ms)
{
    const bool raw_pressed = pin_is_pressed(button);
    uint32_t events = SECDB_BUTTON_EVENT_NONE;

    if (raw_pressed != button->raw_pressed) {
        button->raw_pressed = raw_pressed;
        button->raw_change_ms = now_ms;
    }

    if ((raw_pressed != button->stable_pressed) &&
        ((uint32_t)(now_ms - button->raw_change_ms) >=
         SECDB_BUTTON_DEBOUNCE_MS)) {
        button->stable_pressed = raw_pressed;
        if (raw_pressed) {
            button->press_start_ms = now_ms;
            button->last_repeat_ms = now_ms;
            events |= button->press_event;
        }
    }

    if (button->stable_pressed && (button->repeat_event != 0UL) &&
        ((uint32_t)(now_ms - button->press_start_ms) >=
         SECDB_BUTTON_REPEAT_DELAY_MS) &&
        ((uint32_t)(now_ms - button->last_repeat_ms) >=
         SECDB_BUTTON_REPEAT_RATE_MS)) {
        button->last_repeat_ms = now_ms;
        events |= button->repeat_event;
    }
    return events;
}

void SECDB_Buttons_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t now_ms;
    uint32_t index;

    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin = SECDB_BTN_MODE_Pin | SECDB_BTN_UP_Pin | SECDB_BTN_DOWN_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL; /* The board has external 10 kohm pull-ups. */
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);

    buttons[0].port = SECDB_BTN_MODE_GPIO_Port;
    buttons[0].pin = SECDB_BTN_MODE_Pin;
    buttons[0].press_event = SECDB_BUTTON_EVENT_MODE_PRESS;
    buttons[0].repeat_event = 0UL;

    buttons[1].port = SECDB_BTN_UP_GPIO_Port;
    buttons[1].pin = SECDB_BTN_UP_Pin;
    buttons[1].press_event = SECDB_BUTTON_EVENT_UP_PRESS;
    buttons[1].repeat_event = SECDB_BUTTON_EVENT_UP_REPEAT;

    buttons[2].port = SECDB_BTN_DOWN_GPIO_Port;
    buttons[2].pin = SECDB_BTN_DOWN_Pin;
    buttons[2].press_event = SECDB_BUTTON_EVENT_DOWN_PRESS;
    buttons[2].repeat_event = SECDB_BUTTON_EVENT_DOWN_REPEAT;

    now_ms = HAL_GetTick();
    for (index = 0UL; index < 3UL; ++index) {
        buttons[index].raw_pressed = pin_is_pressed(&buttons[index]);
        buttons[index].stable_pressed = buttons[index].raw_pressed;
        buttons[index].raw_change_ms = now_ms;
        buttons[index].press_start_ms = now_ms;
        buttons[index].last_repeat_ms = now_ms;
    }
}

uint32_t SECDB_Buttons_Poll(void)
{
    const uint32_t now_ms = HAL_GetTick();
    uint32_t events = SECDB_BUTTON_EVENT_NONE;
    uint32_t index;

    for (index = 0UL; index < 3UL; ++index) {
        events |= poll_one(&buttons[index], now_ms);
    }
    return events;
}
