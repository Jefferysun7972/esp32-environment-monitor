/*
 * UART Sensor Driver - Multi-parameter Environmental Sensor Module
 *
 * Communication: UART2, 9600 baud, 8N1
 * Pinout:        TX=GPIO17, RX=GPIO16
 *
 * Data Frame (44 bytes total):
 *   [0x2D 0x23] [len_lo len_hi] [19×uint16_t LE] [cs_lo cs_hi]
 *   Checksum: sum of all bytes from header through data, as uint16_t LE
 */

#ifndef UART_SENSOR_H
#define UART_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================ */
/* PIN & UART CONFIGURATION                     */
/* ============================================ */
#define UART_SENSOR_PORT        UART_NUM_2
#define UART_SENSOR_TX          26
#define UART_SENSOR_RX          27
#define UART_SENSOR_BAUD        9600

/* ============================================ */
/* FRAME CONSTANTS                              */
/* ============================================ */
#define UART_HEAD_LO            0x2D
#define UART_HEAD_HI            0x23
#define UART_HEADER_LEN         2
#define UART_LEN_LEN            2
#define UART_CS_LEN             2
#define UART_FIELD_COUNT        19
#define UART_MAX_FRAME          128

/* ============================================ */
/* SENSOR DATA STRUCTURE (19 × uint16_t LE)     */
/* ============================================ */
typedef struct {
    uint16_t pms_in_pm1_0;       /* PMS_in_data_Spm1_0       */
    uint16_t pms_in_pm2_5;       /* PMS_in_data_Spm2_5       */
    uint16_t pms_in_pm10;        /* PMS_in_data_Spm10        */
    uint16_t pms_out_pm1_0;      /* PMS_out_data_Spm1_0      */
    uint16_t pms_out_pm2_5;      /* PMS_out_data_Spm2_5      */
    uint16_t pms_out_pm10;       /* PMS_out_data_Spm10       */
    uint16_t temperature;        /* G_Temperature (SHT41)    */
    uint16_t humidity;           /* G_Humidity (SHT41)       */
    uint16_t tvoc_count;         /* TVOC_Count (SGP40 VOC)   */
    uint16_t aq_state;           /* G_AQ_State (0-4)         */
    uint16_t audio_state;        /* G_AUDIO_State            */
    uint16_t pir_state;          /* G_PIR_State              */
    uint16_t co2_count;          /* G_CO2_Count (ppm)        */
    uint16_t light_state;        /* G_Light_State            */
    uint16_t pressure_count;     /* G_Pressure_Count (hPa)   */
    uint16_t reserved1;          /* Query_CM1106_ABC_status  */
    uint16_t reserved2;          /* voc_index                */
    uint16_t reserved3;          /* CM1107_calibration_state */
    uint16_t ave_state_sum;      /* averaged_state_sum       */
} uart_sensor_data_t;

/* ============================================ */
/* DRIVER API                                   */
/* ============================================ */

/**
 * @brief Initialize UART for sensor communication
 * @return true on success
 */
bool uart_sensor_init(void);

/**
 * @brief Read one complete data frame from the sensor
 *
 * Blocking read with frame synchronization and checksum validation.
 *
 * @param data  Pointer to data structure to fill
 * @return true if a valid frame was received and parsed
 */
bool uart_sensor_read(uart_sensor_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* UART_SENSOR_H */