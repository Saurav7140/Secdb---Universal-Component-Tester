#ifndef SECDB_CALIBRATION_STORE_H
#define SECDB_CALIBRATION_STORE_H

#include "secdb_bjt.h"
#include "secdb_resistor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SECDB_CALIBRATION_MAGIC    0x53454344UL
#define SECDB_CALIBRATION_VERSION  1U

typedef struct {
    secdb_resistor_calibration_t resistor;
    uint32_t capacitor_timing_denominator_milliohm;
    secdb_bjt_calibration_t bjt;
    uint16_t diode_adc_full_scale_mv;
    uint16_t reserved;
} secdb_calibration_payload_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t image_size;
    uint32_t sequence;
    secdb_calibration_payload_t payload;
    uint32_t crc32;
} secdb_calibration_image_t;

typedef enum {
    SECDB_CALIBRATION_IMAGE_VALID = 0,
    SECDB_CALIBRATION_IMAGE_DEFAULTED,
    SECDB_CALIBRATION_IMAGE_INVALID_ARGUMENT,
    SECDB_CALIBRATION_IMAGE_BAD_HEADER,
    SECDB_CALIBRATION_IMAGE_BAD_CRC,
    SECDB_CALIBRATION_IMAGE_BAD_PAYLOAD,
    SECDB_CALIBRATION_IMAGE_IO_FAILURE
} secdb_calibration_image_status_t;

typedef bool (*secdb_calibration_read_fn)(void *data, size_t size);
typedef bool (*secdb_calibration_write_fn)(const void *data, size_t size);

void SECDB_Calibration_BindStorage(
    secdb_calibration_read_fn read_function,
    secdb_calibration_write_fn write_function);

void SECDB_Calibration_CreateDefaultImage(
    secdb_calibration_image_t *image);

secdb_calibration_image_status_t SECDB_Calibration_FinalizeImage(
    secdb_calibration_image_t *image);

secdb_calibration_image_status_t SECDB_Calibration_LoadImage(
    const void *stored_data,
    size_t stored_size,
    secdb_calibration_image_t *active_image);

secdb_calibration_image_status_t SECDB_Calibration_Load(
    secdb_calibration_image_t *active_image);

secdb_calibration_image_status_t SECDB_Calibration_Save(
    secdb_calibration_image_t *image);

bool SECDB_Calibration_PayloadIsPlausible(
    const secdb_calibration_payload_t *payload);

#endif /* SECDB_CALIBRATION_STORE_H */
