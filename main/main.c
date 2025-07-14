#include "crsf_bridge.h"

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting CRSF Command Bridge Application");
    ESP_LOGI(TAG, "Target: ESP32-S3 with ESP-IDF v5.1.6");
    ESP_LOGI(TAG, "RF Module: RadioMaster Nomad (ExpressLRS v3.5.6)");
    ESP_LOGI(TAG, "Protocol: CRSF v3 (Crossfire Serial Protocol)");
    
    // Initialize the CRSF bridge
    esp_err_t ret = crsf_bridge_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CRSF bridge: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "CRSF Command Bridge is running...");
    ESP_LOGI(TAG, "Host UART: %d baud, TX=%d, RX=%d", 
             HOST_UART_BAUD_RATE, HOST_UART_TX_PIN, HOST_UART_RX_PIN);
    ESP_LOGI(TAG, "CRSF UART: %d baud, TX=%d, RX=%d (Half-duplex)", 
             CRSF_UART_BAUD_RATE, CRSF_UART_TX_PIN, CRSF_UART_RX_PIN);
    
    ESP_LOGI(TAG, "Supported commands:");
    ESP_LOGI(TAG, "  SET_FREQ:<mode> - Set Nomad Frequency Mode");
    ESP_LOGI(TAG, "  SET_PWR:<level> - Set Nomad Power Output Level");
    ESP_LOGI(TAG, "  SET_BIND - Put Nomad into Binding Mode");
    ESP_LOGI(TAG, "  GET_TELEM - Request latest telemetry data");
    ESP_LOGI(TAG, "  INFO - Return Nomad configuration");
    
    // Main loop - keep the application running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}