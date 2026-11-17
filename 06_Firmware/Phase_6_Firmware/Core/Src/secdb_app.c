#include "secdb_app.h"
#include "secdb_afe.h"
#include "secdb_board.h"
#include "secdb_buttons.h"
#include "secdb_log.h"
#include "secdb_ui.h"
#include "stm32f4xx_hal.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SECDB_UI_TEXT_WIDTH  21U

static const uint32_t resistor_values[] = {
    0UL,100UL,220UL,470UL,1000UL,2200UL,4700UL,10000UL,
    22000UL,47000UL,100000UL,220000UL,470000UL,1000000UL
};
static const uint32_t capacitor_values_nf[] = {
    0UL,100UL,220UL,470UL,1000UL,2200UL,4700UL,
    10000UL,22000UL,47000UL,100000UL
};
static const uint16_t tolerance_values_bp[] = {100U,200U,500U,1000U,2000U};
static const uint32_t bjt_minimum_values_x10[] = {
    0UL,200UL,300UL,500UL,1000UL,2000UL,3000UL,4000UL
};
static const uint32_t bjt_maximum_values_x10[] = {
    500UL,1000UL,2000UL,3000UL,4000UL,5000UL
};

static const char *mode_name(secdb_app_mode_t mode)
{
    switch (mode) {
    case SECDB_APP_MODE_RESISTOR: return "RESISTOR";
    case SECDB_APP_MODE_CAPACITOR: return "CAPACITOR";
    case SECDB_APP_MODE_DIODE: return "DIODE";
    case SECDB_APP_MODE_BJT_NPN: return "BJT NPN";
    case SECDB_APP_MODE_BJT_PNP: return "BJT PNP";
    default: return "UNKNOWN";
    }
}

static void clear_lines(secdb_app_t *app)
{
    uint32_t index;
    for (index = 0UL; index < 7UL; ++index) {
        app->ui_lines[index][0] = '\0';
    }
}

static uint8_t wrap_text(secdb_app_t *app, uint8_t start_line,
                         uint8_t maximum_lines, const char *text)
{
    uint8_t line = start_line;
    size_t remaining;
    size_t take;
    size_t break_at;

    if (text == NULL) return line;
    while ((*text != '\0') && (line < (uint8_t)(start_line + maximum_lines)) &&
           (line < 7U)) {
        while (*text == ' ') ++text;
        remaining = strlen(text);
        take = (remaining > SECDB_UI_TEXT_WIDTH) ? SECDB_UI_TEXT_WIDTH
                                                : remaining;
        if (remaining > SECDB_UI_TEXT_WIDTH) {
            break_at = take;
            while ((break_at > 0U) && (text[break_at] != ' ')) --break_at;
            if (break_at > 0U) take = break_at;
        }
        memcpy(app->ui_lines[line], text, take);
        app->ui_lines[line][take] = '\0';
        text += take;
        ++line;
    }
    return line;
}

static void render_frame(secdb_app_t *app, const char *title,
                         secdb_diagnostic_severity_t severity)
{
    secdb_ui_frame_t frame;
    uint32_t index;
    frame.title = title;
    frame.severity = severity;
    for (index = 0UL; index < 7UL; ++index) {
        frame.lines[index] = (app->ui_lines[index][0] == '\0')
            ? NULL : app->ui_lines[index];
    }
    app->display_status = SECDB_UI_Render(&frame);
    if (app->display_status != SECDB_ST7735_OK) {
        app->state = SECDB_APP_STATE_DISPLAY_ERROR;
        SECDB_AFE_EnterSafeTransition();
    }
}

static void render_adc_calibration(secdb_app_t *app)
{
    clear_lines(app);
    (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
                   "REMOVE ALL PARTS");
    (void)snprintf(app->ui_lines[1], sizeof(app->ui_lines[1]),
                   "MODE: ADC CAL");
    if (app->last_adc_calibration_status != SECDB_ADC_OK) {
        (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]),
                       "LAST ERROR: %u",
                       (unsigned int)app->last_adc_calibration_status);
    }
    if (app->calibration_load_status != SECDB_CALIBRATION_IMAGE_VALID) {
        (void)snprintf(app->ui_lines[5], sizeof(app->ui_lines[5]),
                       "DEFAULT PATH CAL");
    }
    render_frame(app, "STARTUP", SECDB_DIAG_SEVERITY_INFORMATION);
}

static void render_menu(secdb_app_t *app)
{
    clear_lines(app);
    (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
                   "TEST: %s", mode_name(app->selected_mode));
    (void)snprintf(app->ui_lines[2], sizeof(app->ui_lines[2]),
                   "UP/DOWN: SELECT");
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]),
                   "MODE: CONTINUE");
    (void)snprintf(app->ui_lines[5], sizeof(app->ui_lines[5]),
                   "UNPOWERED PARTS ONLY");
    render_frame(app, "COMPONENT TEST", SECDB_DIAG_SEVERITY_INFORMATION);
}

static const uint32_t *active_value_table(secdb_app_t *app, size_t *count)
{
    if (app->selected_mode == SECDB_APP_MODE_RESISTOR) {
        *count = sizeof(resistor_values) / sizeof(resistor_values[0]);
        return resistor_values;
    }
    *count = sizeof(capacitor_values_nf) / sizeof(capacitor_values_nf[0]);
    return capacitor_values_nf;
}

static void render_value_reference(secdb_app_t *app)
{
    const uint32_t *values;
    size_t count;
    values = active_value_table(app, &count);
    if (app->reference_value_index >= count) app->reference_value_index = 0U;
    clear_lines(app);
    if (values[app->reference_value_index] == 0UL) {
        (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]), "NO REFERENCE");
    } else {
        (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
            "NOMINAL: %lu %s", (unsigned long)values[app->reference_value_index],
            (app->selected_mode == SECDB_APP_MODE_RESISTOR) ? "OHM" : "NF");
    }
    (void)snprintf(app->ui_lines[2], sizeof(app->ui_lines[2]), "UP/DOWN: CHANGE");
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]), "MODE: ACCEPT");
    render_frame(app, mode_name(app->selected_mode),
                 SECDB_DIAG_SEVERITY_INFORMATION);
}

static void render_tolerance(secdb_app_t *app)
{
    clear_lines(app);
    (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
        "TOLERANCE: %u.%02u%%",
        tolerance_values_bp[app->tolerance_index] / 100U,
        tolerance_values_bp[app->tolerance_index] % 100U);
    (void)snprintf(app->ui_lines[2], sizeof(app->ui_lines[2]), "UP/DOWN: CHANGE");
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]), "MODE: ACCEPT");
    render_frame(app, mode_name(app->selected_mode),
                 SECDB_DIAG_SEVERITY_INFORMATION);
}

static void render_bjt_reference(secdb_app_t *app, bool maximum)
{
    clear_lines(app);
    if (!maximum &&
        (bjt_minimum_values_x10[app->bjt_minimum_index] == 0UL)) {
        (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]), "NO GAIN REFERENCE");
    } else if (maximum) {
        (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
            "MAX HFE: %lu.%lu",
            (unsigned long)(bjt_maximum_values_x10[app->bjt_maximum_index] / 10UL),
            (unsigned long)(bjt_maximum_values_x10[app->bjt_maximum_index] % 10UL));
    } else {
        (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
            "MIN HFE: %lu.%lu",
            (unsigned long)(bjt_minimum_values_x10[app->bjt_minimum_index] / 10UL),
            (unsigned long)(bjt_minimum_values_x10[app->bjt_minimum_index] % 10UL));
    }
    (void)snprintf(app->ui_lines[2], sizeof(app->ui_lines[2]), "UP/DOWN: CHANGE");
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]), "MODE: ACCEPT");
    render_frame(app, mode_name(app->selected_mode),
                 SECDB_DIAG_SEVERITY_INFORMATION);
}

static void render_diode_calibration(secdb_app_t *app)
{
    clear_lines(app);
    (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]), "EMPTY DIODE SOCKET");
    (void)snprintf(app->ui_lines[1], sizeof(app->ui_lines[1]), "MODE: CALIBRATE");
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]), "DO NOT INSERT PART");
    render_frame(app, "DIODE OPEN CAL", SECDB_DIAG_SEVERITY_CAUTION);
}

static void render_ready(secdb_app_t *app)
{
    clear_lines(app);
    (void)snprintf(app->ui_lines[0], sizeof(app->ui_lines[0]),
                   "INSERT %s", mode_name(app->selected_mode));
    if ((app->selected_mode == SECDB_APP_MODE_BJT_NPN) ||
        (app->selected_mode == SECDB_APP_MODE_BJT_PNP)) {
        (void)snprintf(app->ui_lines[1], sizeof(app->ui_lines[1]), "PIN ORDER: E B C");
    }
    if (app->selected_mode == SECDB_APP_MODE_CAPACITOR) {
        (void)snprintf(app->ui_lines[1], sizeof(app->ui_lines[1]), "DISCHARGE FIRST");
    }
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]), "MODE: START TEST");
    (void)snprintf(app->ui_lines[5], sizeof(app->ui_lines[5]), "NO EXTERNAL POWER");
    render_frame(app, "READY", SECDB_DIAG_SEVERITY_CAUTION);
}

static void render_measuring(secdb_app_t *app)
{
    clear_lines(app);
    (void)snprintf(app->ui_lines[1], sizeof(app->ui_lines[1]), "MEASUREMENT ACTIVE");
    (void)snprintf(app->ui_lines[3], sizeof(app->ui_lines[3]), "PLEASE WAIT");
    render_frame(app, mode_name(app->selected_mode),
                 SECDB_DIAG_SEVERITY_INFORMATION);
}

static void render_result(secdb_app_t *app)
{
    uint8_t next_line = 0U;
    clear_lines(app);
    if (app->diagnostic.measured_value_valid) {
        if (app->diagnostic.unit == SECDB_DIAG_UNIT_HFE_X10) {
            (void)snprintf(app->ui_lines[next_line++], sizeof(app->ui_lines[0]),
                "HFE: %lu.%lu",
                (unsigned long)(app->diagnostic.measured_value / 10UL),
                (unsigned long)(app->diagnostic.measured_value % 10UL));
        } else {
            (void)snprintf(app->ui_lines[next_line++], sizeof(app->ui_lines[0]),
                "VALUE: %lu %s", (unsigned long)app->diagnostic.measured_value,
                SECDB_Diagnostic_UnitText(app->diagnostic.unit));
        }
    }
    next_line = wrap_text(app, next_line, 3U,
                          SECDB_Diagnostic_CodeText(app->diagnostic.code));
    (void)wrap_text(app, next_line, (uint8_t)(6U - next_line),
                    SECDB_Diagnostic_ActionText(app->diagnostic.action));
    (void)snprintf(app->ui_lines[6], sizeof(app->ui_lines[6]),
                   "MODE: MENU UP: RETEST");
    render_frame(app, "RESULT", app->diagnostic.severity);
}

static void render_safety_lock(secdb_app_t *app)
{
    clear_lines(app);
    (void)wrap_text(app, 0U, 3U,
                    SECDB_Diagnostic_CodeText(app->diagnostic.code));
    (void)wrap_text(app, 3U, 3U,
                    SECDB_Diagnostic_ActionText(app->diagnostic.action));
    (void)snprintf(app->ui_lines[6], sizeof(app->ui_lines[6]), "SERVICE ACK REQUIRED");
    render_frame(app, "SAFETY LOCK", SECDB_DIAG_SEVERITY_SAFETY);
}

static void advance_index(uint8_t *index, size_t count, bool upward)
{
    if (upward) {
        *index = (uint8_t)((*index + 1U) % count);
    } else {
        *index = (*index == 0U) ? (uint8_t)(count - 1U)
                               : (uint8_t)(*index - 1U);
    }
}

static void enter_configuration(secdb_app_t *app)
{
    app->value_reference.enabled = false;
    app->bjt_gain_reference.enabled = false;
    if ((app->selected_mode == SECDB_APP_MODE_RESISTOR) ||
        (app->selected_mode == SECDB_APP_MODE_CAPACITOR)) {
        app->state = SECDB_APP_STATE_VALUE_REFERENCE;
        render_value_reference(app);
    } else if (app->selected_mode == SECDB_APP_MODE_DIODE) {
        app->state = app->diode_open_calibration.valid
            ? SECDB_APP_STATE_READY
            : SECDB_APP_STATE_DIODE_OPEN_CALIBRATION;
        if (app->state == SECDB_APP_STATE_READY) render_ready(app);
        else render_diode_calibration(app);
    } else {
        app->state = SECDB_APP_STATE_BJT_GAIN_MINIMUM;
        render_bjt_reference(app, false);
    }
}

static void emit_result_log(secdb_app_t *app)
{
    ++app->log_sequence;
    switch (app->selected_mode) {
    case SECDB_APP_MODE_RESISTOR:
        app->last_log_succeeded = SECDB_Log_Resistor(
            app->log_sequence, &app->resistor_result, &app->diagnostic);
        break;
    case SECDB_APP_MODE_CAPACITOR:
        app->last_log_succeeded = SECDB_Log_Capacitor(
            app->log_sequence, &app->capacitor_result, &app->diagnostic);
        break;
    case SECDB_APP_MODE_DIODE:
        app->last_log_succeeded = SECDB_Log_Diode(
            app->log_sequence, &app->diode_result, &app->diagnostic);
        break;
    case SECDB_APP_MODE_BJT_NPN:
    case SECDB_APP_MODE_BJT_PNP:
        app->last_log_succeeded = SECDB_Log_Bjt(
            app->log_sequence, &app->bjt_result, &app->diagnostic);
        break;
    default:
        app->last_log_succeeded = false;
        break;
    }
}

static void perform_measurement(secdb_app_t *app)
{
    render_measuring(app);
    if (app->state == SECDB_APP_STATE_DISPLAY_ERROR) return;
    app->state = SECDB_APP_STATE_MEASURING;
    HAL_GPIO_WritePin(SECDB_LED_GPIO_Port, SECDB_LED_Pin, GPIO_PIN_SET);

    switch (app->selected_mode) {
    case SECDB_APP_MODE_RESISTOR:
        (void)SECDB_Resistor_Measure(
            &app->calibration.payload.resistor, &app->resistor_result);
        (void)SECDB_Diagnostic_FromResistor(
            &app->resistor_result, &app->value_reference, &app->diagnostic);
        break;
    case SECDB_APP_MODE_CAPACITOR:
        (void)SECDB_Capacitor_Measure(
            app->calibration.payload.capacitor_timing_denominator_milliohm,
            &app->capacitor_result);
        (void)SECDB_Diagnostic_FromCapacitor(
            &app->capacitor_result, &app->value_reference, &app->diagnostic);
        break;
    case SECDB_APP_MODE_DIODE:
        (void)SECDB_Diode_Measure(
            &app->diode_open_calibration, &app->diode_result);
        (void)SECDB_Diagnostic_FromDiode(
            &app->diode_result, &app->diagnostic);
        break;
    case SECDB_APP_MODE_BJT_NPN:
    case SECDB_APP_MODE_BJT_PNP:
        (void)SECDB_BJT_Measure(
            (app->selected_mode == SECDB_APP_MODE_BJT_NPN)
                ? SECDB_BJT_NPN : SECDB_BJT_PNP,
            &app->calibration.payload.bjt, &app->bjt_result);
        (void)SECDB_Diagnostic_FromBjt(
            &app->bjt_result, &app->bjt_gain_reference, &app->diagnostic);
        break;
    default:
        SECDB_AFE_EnterSafeTransition();
        HAL_GPIO_WritePin(SECDB_LED_GPIO_Port, SECDB_LED_Pin, GPIO_PIN_RESET);
        app->state = SECDB_APP_STATE_MENU;
        render_menu(app);
        return;
    }

    HAL_GPIO_WritePin(SECDB_LED_GPIO_Port, SECDB_LED_Pin, GPIO_PIN_RESET);
    emit_result_log(app);
    if (app->diagnostic.severity == SECDB_DIAG_SEVERITY_SAFETY) {
        app->state = SECDB_APP_STATE_SAFETY_LOCK;
        render_safety_lock(app);
    } else {
        app->state = SECDB_APP_STATE_RESULT;
        render_result(app);
    }
}

secdb_app_status_t SECDB_App_Init(secdb_app_t *app)
{
    GPIO_InitTypeDef gpio = {0};
    if (app == NULL) return SECDB_APP_INVALID_ARGUMENT;
    memset(app, 0, sizeof(*app));
    if (!SECDB_ST7735_IsReady()) {
        app->state = SECDB_APP_STATE_DISPLAY_ERROR;
        return SECDB_APP_DISPLAY_NOT_READY;
    }

    app->calibration_load_status = SECDB_Calibration_Load(&app->calibration);
    app->selected_mode = SECDB_APP_MODE_RESISTOR;
    app->state = SECDB_APP_STATE_ADC_CALIBRATION;
    app->last_adc_calibration_status = SECDB_ADC_OK;
    app->tolerance_index = 2U; /* 5.00 percent */
    app->last_log_succeeded = SECDB_Log_WriteHeader();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = SECDB_LED_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SECDB_LED_GPIO_Port, &gpio);
    HAL_GPIO_WritePin(SECDB_LED_GPIO_Port, SECDB_LED_Pin, GPIO_PIN_RESET);
    SECDB_Buttons_Init();
    SECDB_AFE_EnterSafeTransition();
    render_adc_calibration(app);
    return (app->state == SECDB_APP_STATE_DISPLAY_ERROR)
        ? SECDB_APP_DISPLAY_NOT_READY : SECDB_APP_OK;
}

static void handle_adjustment(secdb_app_t *app, bool upward)
{
    size_t count;
    uint32_t minimum;
    switch (app->state) {
    case SECDB_APP_STATE_MENU:
        if (upward) {
            app->selected_mode = (secdb_app_mode_t)(
                ((uint32_t)app->selected_mode + 1UL) % SECDB_APP_MODE_COUNT);
        } else {
            app->selected_mode = (app->selected_mode == SECDB_APP_MODE_RESISTOR)
                ? (secdb_app_mode_t)(SECDB_APP_MODE_COUNT - 1U)
                : (secdb_app_mode_t)((uint32_t)app->selected_mode - 1UL);
        }
        render_menu(app);
        break;
    case SECDB_APP_STATE_VALUE_REFERENCE:
        (void)active_value_table(app, &count);
        advance_index(&app->reference_value_index, count, upward);
        render_value_reference(app);
        break;
    case SECDB_APP_STATE_TOLERANCE:
        count = sizeof(tolerance_values_bp) / sizeof(tolerance_values_bp[0]);
        advance_index(&app->tolerance_index, count, upward);
        render_tolerance(app);
        break;
    case SECDB_APP_STATE_BJT_GAIN_MINIMUM:
        count = sizeof(bjt_minimum_values_x10) /
                sizeof(bjt_minimum_values_x10[0]);
        advance_index(&app->bjt_minimum_index, count, upward);
        render_bjt_reference(app, false);
        break;
    case SECDB_APP_STATE_BJT_GAIN_MAXIMUM:
        count = sizeof(bjt_maximum_values_x10) /
                sizeof(bjt_maximum_values_x10[0]);
        advance_index(&app->bjt_maximum_index, count, upward);
        minimum = bjt_minimum_values_x10[app->bjt_minimum_index];
        while (bjt_maximum_values_x10[app->bjt_maximum_index] < minimum) {
            advance_index(&app->bjt_maximum_index, count, true);
        }
        render_bjt_reference(app, true);
        break;
    case SECDB_APP_STATE_RESULT:
        if (upward) {
            app->state = SECDB_APP_STATE_READY;
            render_ready(app);
        } else {
            app->state = SECDB_APP_STATE_MENU;
            render_menu(app);
        }
        break;
    default:
        break;
    }
}

static void handle_mode_press(secdb_app_t *app)
{
    const uint32_t *values;
    size_t count;
    uint32_t minimum;
    switch (app->state) {
    case SECDB_APP_STATE_ADC_CALIBRATION:
        app->last_adc_calibration_status =
            SECDB_ADC_RunCalibration(&app->adc_calibration);
        if (app->last_adc_calibration_status == SECDB_ADC_OK) {
            app->state = SECDB_APP_STATE_MENU;
            render_menu(app);
        } else {
            render_adc_calibration(app);
        }
        break;
    case SECDB_APP_STATE_MENU:
        enter_configuration(app);
        break;
    case SECDB_APP_STATE_VALUE_REFERENCE:
        values = active_value_table(app, &count);
        (void)count;
        app->value_reference.enabled =
            values[app->reference_value_index] != 0UL;
        app->value_reference.nominal_value =
            values[app->reference_value_index];
        if (app->value_reference.enabled) {
            app->state = SECDB_APP_STATE_TOLERANCE;
            render_tolerance(app);
        } else {
            app->state = SECDB_APP_STATE_READY;
            render_ready(app);
        }
        break;
    case SECDB_APP_STATE_TOLERANCE:
        app->value_reference.tolerance_basis_points =
            tolerance_values_bp[app->tolerance_index];
        app->state = SECDB_APP_STATE_READY;
        render_ready(app);
        break;
    case SECDB_APP_STATE_BJT_GAIN_MINIMUM:
        minimum = bjt_minimum_values_x10[app->bjt_minimum_index];
        app->bjt_gain_reference.enabled = minimum != 0UL;
        app->bjt_gain_reference.minimum_hfe_x10 = minimum;
        if (!app->bjt_gain_reference.enabled) {
            app->state = SECDB_APP_STATE_READY;
            render_ready(app);
        } else {
            count = sizeof(bjt_maximum_values_x10) /
                    sizeof(bjt_maximum_values_x10[0]);
            app->bjt_maximum_index = 0U;
            while (bjt_maximum_values_x10[app->bjt_maximum_index] < minimum) {
                advance_index(&app->bjt_maximum_index, count, true);
            }
            app->state = SECDB_APP_STATE_BJT_GAIN_MAXIMUM;
            render_bjt_reference(app, true);
        }
        break;
    case SECDB_APP_STATE_BJT_GAIN_MAXIMUM:
        app->bjt_gain_reference.maximum_hfe_x10 =
            bjt_maximum_values_x10[app->bjt_maximum_index];
        app->state = SECDB_APP_STATE_READY;
        render_ready(app);
        break;
    case SECDB_APP_STATE_DIODE_OPEN_CALIBRATION:
        if (SECDB_Diode_RunOpenCalibration(
                app->calibration.payload.diode_adc_full_scale_mv,
                &app->diode_open_calibration) == SECDB_DIODE_CALIBRATION_READY) {
            app->state = SECDB_APP_STATE_READY;
            render_ready(app);
        } else {
            render_diode_calibration(app);
        }
        break;
    case SECDB_APP_STATE_READY:
        perform_measurement(app);
        break;
    case SECDB_APP_STATE_RESULT:
        app->state = SECDB_APP_STATE_MENU;
        render_menu(app);
        break;
    default:
        break;
    }
}

void SECDB_App_Tick(secdb_app_t *app)
{
    if ((app == NULL) || (app->state == SECDB_APP_STATE_SAFETY_LOCK) ||
        (app->state == SECDB_APP_STATE_DISPLAY_ERROR) ||
        (app->state == SECDB_APP_STATE_MEASURING)) {
        return;
    }
    SECDB_App_ProcessButtonEvents(app, SECDB_Buttons_Poll());
}

void SECDB_App_ProcessButtonEvents(secdb_app_t *app, uint32_t events)
{
    if ((app == NULL) || (app->state == SECDB_APP_STATE_SAFETY_LOCK) ||
        (app->state == SECDB_APP_STATE_DISPLAY_ERROR) ||
        (app->state == SECDB_APP_STATE_MEASURING)) {
        return;
    }
    if ((events & SECDB_BUTTON_EVENT_MODE_PRESS) != 0UL) {
        handle_mode_press(app);
    } else if ((events & (SECDB_BUTTON_EVENT_UP_PRESS |
                          SECDB_BUTTON_EVENT_UP_REPEAT)) != 0UL) {
        handle_adjustment(app, true);
    } else if ((events & (SECDB_BUTTON_EVENT_DOWN_PRESS |
                          SECDB_BUTTON_EVENT_DOWN_REPEAT)) != 0UL) {
        handle_adjustment(app, false);
    }
}

secdb_app_status_t SECDB_App_ClearSafetyLock(
    secdb_app_t *app,
    bool user_confirmed_component_removed_and_discharged)
{
    if (app == NULL) return SECDB_APP_INVALID_ARGUMENT;
    if ((app->state != SECDB_APP_STATE_SAFETY_LOCK) ||
        !user_confirmed_component_removed_and_discharged) {
        return SECDB_APP_SAFETY_CONFIRMATION_REQUIRED;
    }
    SECDB_AFE_EnterSafeTransition();
    app->state = SECDB_APP_STATE_MENU;
    render_menu(app);
    return SECDB_APP_OK;
}

secdb_app_status_t SECDB_App_SaveCalibration(secdb_app_t *app)
{
    if (app == NULL) return SECDB_APP_INVALID_ARGUMENT;
    return (SECDB_Calibration_Save(&app->calibration) ==
            SECDB_CALIBRATION_IMAGE_VALID)
        ? SECDB_APP_OK : SECDB_APP_STORAGE_FAILURE;
}

secdb_calibration_image_t *SECDB_App_GetCalibrationForService(
    secdb_app_t *app)
{
    return (app == NULL) ? NULL : &app->calibration;
}
