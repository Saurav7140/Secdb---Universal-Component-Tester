#ifndef SECDB_UI_H
#define SECDB_UI_H

#include "secdb_diagnostic.h"
#include "secdb_st7735.h"
#include <stdint.h>

#define SECDB_UI_LINE_COUNT  7U

typedef struct {
    const char *title;
    const char *lines[SECDB_UI_LINE_COUNT];
    secdb_diagnostic_severity_t severity;
} secdb_ui_frame_t;

secdb_st7735_status_t SECDB_UI_Render(const secdb_ui_frame_t *frame);

#endif /* SECDB_UI_H */
