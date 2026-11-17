#include "secdb_calibration_store.h"
#include "secdb_capacitor.h"
#include <stddef.h>
#include <string.h>

static secdb_calibration_read_fn storage_read;
static secdb_calibration_write_fn storage_write;

void SECDB_Calibration_BindStorage(
    secdb_calibration_read_fn read_function,
    secdb_calibration_write_fn write_function)
{
    storage_read = read_function;
    storage_write = write_function;
}

static uint32_t crc32_bytes(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t byte_index;
    uint32_t bit_index;

    for (byte_index = 0U; byte_index < length; ++byte_index) {
        crc ^= data[byte_index];
        for (bit_index = 0UL; bit_index < 8UL; ++bit_index) {
            crc = (crc & 1UL) ? ((crc >> 1U) ^ 0xEDB88320UL)
                              : (crc >> 1U);
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

bool SECDB_Calibration_PayloadIsPlausible(
    const secdb_calibration_payload_t *payload)
{
    if (payload == NULL) {
        return false;
    }
    return
        (payload->resistor.path_1k_milliohm >= 500000UL) &&
        (payload->resistor.path_1k_milliohm <= 5000000UL) &&
        (payload->resistor.path_10k_milliohm >= 5000000UL) &&
        (payload->resistor.path_10k_milliohm <= 50000000UL) &&
        (payload->resistor.path_100k_milliohm >= 50000000UL) &&
        (payload->resistor.path_100k_milliohm <= 500000000UL) &&
        (payload->capacitor_timing_denominator_milliohm >=
         SECDB_CAP_MIN_TIMING_DENOMINATOR_MOHM) &&
        (payload->capacitor_timing_denominator_milliohm <=
         SECDB_CAP_MAX_TIMING_DENOMINATOR_MOHM) &&
        (payload->bjt.drive_low_mv <= 100U) &&
        (payload->bjt.drive_high_mv >= 3000U) &&
        (payload->bjt.drive_high_mv <= 3600U) &&
        (payload->bjt.drive_high_mv > payload->bjt.drive_low_mv) &&
        (payload->bjt.adc_full_scale_mv >= 3000U) &&
        (payload->bjt.adc_full_scale_mv <= 3600U) &&
        (payload->bjt.base_path_ohm >= 500000UL) &&
        (payload->bjt.base_path_ohm <= 2000000UL) &&
        (payload->bjt.collector_path_ohm >= 500UL) &&
        (payload->bjt.collector_path_ohm <= 2000UL) &&
        (payload->diode_adc_full_scale_mv >= 3000U) &&
        (payload->diode_adc_full_scale_mv <= 3600U);
}

secdb_calibration_image_status_t SECDB_Calibration_FinalizeImage(
    secdb_calibration_image_t *image)
{
    if (image == NULL) {
        return SECDB_CALIBRATION_IMAGE_INVALID_ARGUMENT;
    }
    if (!SECDB_Calibration_PayloadIsPlausible(&image->payload)) {
        return SECDB_CALIBRATION_IMAGE_BAD_PAYLOAD;
    }
    image->magic = SECDB_CALIBRATION_MAGIC;
    image->version = SECDB_CALIBRATION_VERSION;
    image->image_size = (uint16_t)sizeof(*image);
    image->crc32 = crc32_bytes(
        (const uint8_t *)image, offsetof(secdb_calibration_image_t, crc32));
    return SECDB_CALIBRATION_IMAGE_VALID;
}

void SECDB_Calibration_CreateDefaultImage(secdb_calibration_image_t *image)
{
    if (image != NULL) {
        memset(image, 0, sizeof(*image));
        image->payload.resistor = SECDB_Resistor_DefaultCalibration();
        image->payload.capacitor_timing_denominator_milliohm =
            SECDB_Capacitor_DefaultTimingDenominatorMilliohm();
        image->payload.bjt = SECDB_BJT_DefaultCalibration();
        image->payload.diode_adc_full_scale_mv =
            image->payload.bjt.adc_full_scale_mv;
        (void)SECDB_Calibration_FinalizeImage(image);
    }
}

secdb_calibration_image_status_t SECDB_Calibration_LoadImage(
    const void *stored_data,
    size_t stored_size,
    secdb_calibration_image_t *active_image)
{
    secdb_calibration_image_t candidate;
    uint32_t expected_crc;

    if (active_image == NULL) {
        return SECDB_CALIBRATION_IMAGE_INVALID_ARGUMENT;
    }
    SECDB_Calibration_CreateDefaultImage(active_image);
    if ((stored_data == NULL) ||
        (stored_size != sizeof(secdb_calibration_image_t))) {
        return SECDB_CALIBRATION_IMAGE_DEFAULTED;
    }

    memcpy(&candidate, stored_data, sizeof(candidate));
    if ((candidate.magic != SECDB_CALIBRATION_MAGIC) ||
        (candidate.version != SECDB_CALIBRATION_VERSION) ||
        (candidate.image_size != sizeof(candidate))) {
        return SECDB_CALIBRATION_IMAGE_BAD_HEADER;
    }
    expected_crc = crc32_bytes(
        (const uint8_t *)&candidate,
        offsetof(secdb_calibration_image_t, crc32));
    if (expected_crc != candidate.crc32) {
        return SECDB_CALIBRATION_IMAGE_BAD_CRC;
    }
    if (!SECDB_Calibration_PayloadIsPlausible(&candidate.payload)) {
        return SECDB_CALIBRATION_IMAGE_BAD_PAYLOAD;
    }
    *active_image = candidate;
    return SECDB_CALIBRATION_IMAGE_VALID;
}

secdb_calibration_image_status_t SECDB_Calibration_Load(
    secdb_calibration_image_t *active_image)
{
    secdb_calibration_image_t stored_image;
    if (active_image == NULL) {
        return SECDB_CALIBRATION_IMAGE_INVALID_ARGUMENT;
    }
    if (storage_read == NULL) {
        SECDB_Calibration_CreateDefaultImage(active_image);
        return SECDB_CALIBRATION_IMAGE_DEFAULTED;
    }
    if (!storage_read(&stored_image, sizeof(stored_image))) {
        SECDB_Calibration_CreateDefaultImage(active_image);
        return SECDB_CALIBRATION_IMAGE_IO_FAILURE;
    }
    return SECDB_Calibration_LoadImage(
        &stored_image, sizeof(stored_image), active_image);
}

secdb_calibration_image_status_t SECDB_Calibration_Save(
    secdb_calibration_image_t *image)
{
    secdb_calibration_image_status_t status;
    if ((image == NULL) || (storage_write == NULL)) {
        return SECDB_CALIBRATION_IMAGE_INVALID_ARGUMENT;
    }
    ++image->sequence;
    status = SECDB_Calibration_FinalizeImage(image);
    if (status != SECDB_CALIBRATION_IMAGE_VALID) {
        --image->sequence;
        return status;
    }
    if (!storage_write(image, sizeof(*image))) {
        --image->sequence;
        (void)SECDB_Calibration_FinalizeImage(image);
        return SECDB_CALIBRATION_IMAGE_IO_FAILURE;
    }
    return SECDB_CALIBRATION_IMAGE_VALID;
}
