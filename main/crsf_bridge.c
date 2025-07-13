#include "crsf_bridge.h"
#include "serial_commands.h"
#include "crsf_protocol.h"

static const char *TAG = "CRSF_BRIDGE";

// Global variables
QueueHandle_t host_command_queue;
QueueHandle_t crsf_response_queue;
SemaphoreHandle_t telemetry_mutex;
telemetry_data_t latest_telemetry;

// Task handles
TaskHandle_t host_task_handle;
TaskHandle_t crsf_task_handle;
TaskHandle_t telemetry_task_handle;

void host_uart_task(void *pvParameters) {
    ESP_LOGI(TAG, "Host UART task started");
    
    char rx_buffer[HOST_BUF_SIZE];
    char tx_buffer[RESPONSE_BUF_SIZE];
    char cmd_buffer[COMMAND_BUF_SIZE];
    host_command_t cmd;
    
    while (1) {
        // Read data from host UART
        int len = uart_read_bytes(HOST_UART_NUM, rx_buffer, sizeof(rx_buffer) - 1, 
                                 pdMS_TO_TICKS(100));
        
        if (len > 0) {
            rx_buffer[len] = '\0';
            
            // Remove trailing newline/carriage return
            char *newline = strchr(rx_buffer, '\r');
            if (newline) *newline = '\0';
            newline = strchr(rx_buffer, '\n');
            if (newline) *newline = '\0';
            
            ESP_LOGI(TAG, "Received command: %s", rx_buffer);
            
            // Parse command
            if (parse_host_command(rx_buffer, &cmd) == ESP_OK) {
                esp_err_t result = ESP_OK;
                
                // Execute command
                switch (cmd.type) {
                    case CMD_SET_FREQ:
                        result = handle_set_freq_command(cmd.param);
                        break;
                    case CMD_SET_PWR:
                        result = handle_set_pwr_command(cmd.param);
                        break;
                    case CMD_SET_BIND:
                        result = handle_set_bind_command();
                        break;
                    case CMD_GET_TELEM:
                        if (handle_get_telem_command(cmd_buffer, sizeof(cmd_buffer)) == ESP_OK) {
                            format_response(RESP_TELEM, cmd_buffer, tx_buffer, sizeof(tx_buffer));
                        } else {
                            format_response(RESP_ERR_CRSF_FAIL, NULL, tx_buffer, sizeof(tx_buffer));
                        }
                        uart_write_bytes(HOST_UART_NUM, tx_buffer, strlen(tx_buffer));
                        continue;
                    case CMD_INFO:
                        if (handle_info_command(cmd_buffer, sizeof(cmd_buffer)) == ESP_OK) {
                            format_response(RESP_TELEM, cmd_buffer, tx_buffer, sizeof(tx_buffer));
                        } else {
                            format_response(RESP_ERR_CRSF_FAIL, NULL, tx_buffer, sizeof(tx_buffer));
                        }
                        uart_write_bytes(HOST_UART_NUM, tx_buffer, strlen(tx_buffer));
                        continue;
                    default:
                        result = ESP_ERR_INVALID_ARG;
                        break;
                }
                
                // Send response
                if (result == ESP_OK) {
                    format_response(RESP_OK, NULL, tx_buffer, sizeof(tx_buffer));
                } else if (result == ESP_ERR_INVALID_ARG) {
                    format_response(RESP_ERR_BAD_PARAM, NULL, tx_buffer, sizeof(tx_buffer));
                } else {
                    format_response(RESP_ERR_CRSF_FAIL, NULL, tx_buffer, sizeof(tx_buffer));
                }
                
                uart_write_bytes(HOST_UART_NUM, tx_buffer, strlen(tx_buffer));
            } else {
                // Invalid command
                format_response(RESP_ERR_BAD_CMD, NULL, tx_buffer, sizeof(tx_buffer));
                uart_write_bytes(HOST_UART_NUM, tx_buffer, strlen(tx_buffer));
            }
        }
    }
}

void crsf_uart_task(void *pvParameters) {
    ESP_LOGI(TAG, "CRSF UART task started");
    
    uint8_t rx_buffer[CRSF_BUF_SIZE];
    
    while (1) {
        // Read data from CRSF UART
        int len = uart_read_bytes(CRSF_UART_NUM, rx_buffer, sizeof(rx_buffer), 
                                 pdMS_TO_TICKS(100));
        
        if (len > 0) {
            ESP_LOGD(TAG, "Received %d bytes from CRSF UART", len);
            
            // Process received CRSF frame
            crsf_process_received_frame(rx_buffer, len);
        }
        
        // Send periodic telemetry request
        static uint32_t last_telem_request = 0;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - last_telem_request > 1000) { // Every 1 second
            crsf_send_telemetry_request();
            last_telem_request = now;
        }
    }
}

void telemetry_relay_task(void *pvParameters) {
    ESP_LOGI(TAG, "Telemetry relay task started");
    
    while (1) {
        // This task handles automatic telemetry relay if needed
        // For now, it just sleeps and maintains the telemetry data
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Update packet counters (example implementation)
        if (xSemaphoreTake(telemetry_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            latest_telemetry.good_packets++;
            xSemaphoreGive(telemetry_mutex);
        }
    }
}

esp_err_t crsf_bridge_init(void) {
    ESP_LOGI(TAG, "Initializing CRSF bridge");
    
    // Initialize telemetry data
    memset(&latest_telemetry, 0, sizeof(latest_telemetry));
    
    // Create mutex for telemetry data
    telemetry_mutex = xSemaphoreCreateMutex();
    if (telemetry_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create telemetry mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize host UART
    uart_config_t host_uart_config = {
        .baud_rate = HOST_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    esp_err_t ret = uart_driver_install(HOST_UART_NUM, HOST_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install host UART driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = uart_param_config(HOST_UART_NUM, &host_uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure host UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = uart_set_pin(HOST_UART_NUM, HOST_UART_TX_PIN, HOST_UART_RX_PIN, 
                       HOST_UART_RTS_PIN, HOST_UART_CTS_PIN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set host UART pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize CRSF protocol
    ret = crsf_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CRSF protocol: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create tasks
    xTaskCreate(host_uart_task, "host_uart_task", HOST_TASK_STACK_SIZE, NULL, 
                HOST_TASK_PRIORITY, &host_task_handle);
    
    xTaskCreate(crsf_uart_task, "crsf_uart_task", CRSF_TASK_STACK_SIZE, NULL, 
                CRSF_TASK_PRIORITY, &crsf_task_handle);
    
    xTaskCreate(telemetry_relay_task, "telemetry_relay_task", TELEMETRY_TASK_STACK_SIZE, NULL, 
                TELEMETRY_TASK_PRIORITY, &telemetry_task_handle);
    
    ESP_LOGI(TAG, "CRSF bridge initialized successfully");
    return ESP_OK;
}