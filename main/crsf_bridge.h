#ifndef CRSF_BRIDGE_H
#define CRSF_BRIDGE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

// UART Configuration
#define HOST_UART_NUM           UART_NUM_1
#define HOST_UART_BAUD_RATE     115200
#define HOST_UART_TX_PIN        GPIO_NUM_17
#define HOST_UART_RX_PIN        GPIO_NUM_18
#define HOST_UART_RTS_PIN       UART_PIN_NO_CHANGE
#define HOST_UART_CTS_PIN       UART_PIN_NO_CHANGE

#define CRSF_UART_NUM           UART_NUM_2
#define CRSF_UART_BAUD_RATE     420000
#define CRSF_UART_TX_PIN        GPIO_NUM_4
#define CRSF_UART_RX_PIN        GPIO_NUM_5
#define CRSF_UART_RTS_PIN       UART_PIN_NO_CHANGE
#define CRSF_UART_CTS_PIN       UART_PIN_NO_CHANGE

// Buffer sizes
#define HOST_BUF_SIZE           256
#define CRSF_BUF_SIZE           256
#define COMMAND_BUF_SIZE        128
#define RESPONSE_BUF_SIZE       256

// Task priorities
#define HOST_TASK_PRIORITY      5
#define CRSF_TASK_PRIORITY      6
#define TELEMETRY_TASK_PRIORITY 4

// Task stack sizes
#define HOST_TASK_STACK_SIZE    4096
#define CRSF_TASK_STACK_SIZE    4096
#define TELEMETRY_TASK_STACK_SIZE 4096

// Command types
typedef enum {
    CMD_SET_FREQ,
    CMD_SET_PWR,
    CMD_SET_BIND,
    CMD_GET_TELEM,
    CMD_INFO,
    CMD_SEND_CHANNELS,
    CMD_UNKNOWN
} command_type_t;

// Response types
typedef enum {
    RESP_OK,
    RESP_ERR_BAD_CMD,
    RESP_ERR_BAD_PARAM,
    RESP_ERR_CRSF_FAIL,
    RESP_TELEM
} response_type_t;

// Command structure
typedef struct {
    command_type_t type;
    char param[64];
    int param_value;
    uint8_t channel_data[22];  // For RC channel data (16 channels * 11 bits packed)
} host_command_t;

// Telemetry data structure
typedef struct {
    uint8_t rssi;
    uint8_t lq;
    uint8_t snr;
    uint8_t power;
    uint8_t rfmode;
    uint32_t bad_packets;
    uint32_t good_packets;
} telemetry_data_t;

// Global variables
extern QueueHandle_t host_command_queue;
extern QueueHandle_t crsf_response_queue;
extern SemaphoreHandle_t telemetry_mutex;
extern telemetry_data_t latest_telemetry;

// Function prototypes
void host_uart_task(void *pvParameters);
void crsf_uart_task(void *pvParameters);
void telemetry_relay_task(void *pvParameters);

// Bridge initialization
esp_err_t crsf_bridge_init(void);

#endif // CRSF_BRIDGE_H