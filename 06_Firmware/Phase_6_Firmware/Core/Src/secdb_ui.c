#include "secdb_ui.h"
#include <stddef.h>

static uint16_t severity_color(secdb_diagnostic_severity_t severity)
{
    switch (severity) {
    case SECDB_DIAG_SEVERITY_PASS: return SECDB_COLOR_GREEN;
    case SECDB_DIAG_SEVERITY_CAUTION: return SECDB_COLOR_YELLOW;
    case SECDB_DIAG_SEVERITY_FAULT: return SECDB_COLOR_ORANGE;
    case SECDB_DIAG_SEVERITY_SAFETY: return SECDB_COLOR_RED;
    case SECDB_DIAG_SEVERITY_SYSTEM_ERROR: return SECDB_COLOR_RED;
    case SECDB_DIAG_SEVERITY_INFORMATION:
    default: return SECDB_COLOR_CYAN;
    }
}

secdb_st7735_status_t SECDB_UI_Render(const secdb_ui_frame_t *frame)
{
    secdb_st7735_status_t status;
    uint16_t line_index;
    uint16_t y;
    if ((frame == NULL) || (frame->title == NULL)) {
        return SECDB_ST7735_INVALID_ARGUMENT;
    }
    status = SECDB_ST7735_Clear(SECDB_COLOR_BLACK);
    if (status != SECDB_ST7735_OK) return status;
    status = SECDB_ST7735_FillRectangle(
        0U, 0U, SECDB_ST7735_WIDTH, 14U, severity_color(frame->severity));
    if (status != SECDB_ST7735_OK) return status;
    status = SECDB_ST7735_WriteText(
        2U, 3U, frame->title, 1U, SECDB_COLOR_BLACK,
        severity_color(frame->severity));
    if (status != SECDB_ST7735_OK) return status;

    for (line_index = 0U; line_index < SECDB_UI_LINE_COUNT; ++line_index) {
        if (frame->lines[line_index] != NULL) {
            y = (uint16_t)(22U + (line_index * 18U));
            status = SECDB_ST7735_WriteText(
                2U, y, frame->lines[line_index], 1U,
                SECDB_COLOR_WHITE, SECDB_COLOR_BLACK);
            if (status != SECDB_ST7735_OK) return status;
        }
    }
    return SECDB_ST7735_OK;
}
