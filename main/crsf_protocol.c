#include "crsf_protocol.h"
#include "soc/gpio_periph.h"
#include "soc/gpio_sig_map.h"
#include "esp_rom_gpio.h"

static const char *TAG = "CRSF_PROTOCOL";

// Half-duplex control functions
static void crsf_set_tx_mode(void);
static void crsf_set_rx_mode(void);

static uint8_t crsf_crc_table[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9
};

// Half-duplex control functions for one-wire operation
static void crsf_set_tx_mode(void) {
    // Disable interrupts during GPIO matrix reconfiguration
    portDISABLE_INTERRUPTS();
    
    // Set pin as output
    gpio_set_direction(CRSF_UART_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(CRSF_UART_PIN, 1);  // Set high (idle state)
    
    // Disconnect RX from GPIO matrix and connect TX
    gpio_matrix_in(0x30, U2RXD_IN_IDX, false);  // Disconnect RX (route constant 0 to matrix)
    gpio_matrix_out(CRSF_UART_PIN, U2TXD_OUT_IDX, false, false);  // Connect TX to pin
    
    portENABLE_INTERRUPTS();
}

static void crsf_set_rx_mode(void) {
    // Disable interrupts during GPIO matrix reconfiguration
    portDISABLE_INTERRUPTS();
    
    // Set pin as input with pull-up
    gpio_set_direction(CRSF_UART_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(CRSF_UART_PIN);
    gpio_pulldown_dis(CRSF_UART_PIN);
    
    // Connect RX to GPIO matrix and disconnect TX
    gpio_matrix_in(CRSF_UART_PIN, U2RXD_IN_IDX, false);  // Connect RX to pin
    gpio_matrix_out(CRSF_UART_PIN, 0x100, false, false);  // Disconnect TX from pin
    
    portENABLE_INTERRUPTS();
}

uint8_t crsf_calculate_crc(uint8_t *data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc = crsf_crc_table[crc ^ data[i]];
    }
    return crc;
}

bool crsf_validate_frame(crsf_frame_t *frame) {
    if (!frame || frame->sync != CRSF_SYNC_BYTE) {
        return false;
    }
    
    if (frame->length < 2 || frame->length > CRSF_FRAME_SIZE_MAX - 1) {
        return false;
    }
    
    uint8_t calculated_crc = crsf_calculate_crc(&frame->length, frame->length - 1);
    return calculated_crc == frame->crc;
}

esp_err_t crsf_init(void) {
    ESP_LOGI(TAG, "Initializing CRSF protocol");
    
    // Initialize UART for half-duplex operation
    uart_config_t uart_config = {
        .baud_rate = CRSF_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    esp_err_t ret = uart_driver_install(CRSF_UART_NUM, CRSF_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = uart_param_config(CRSF_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // For half-duplex, we don't use uart_set_pin() - we manually configure the GPIO matrix
    // Initialize GPIO pin for half-duplex operation
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CRSF_UART_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Start in RX mode
    crsf_set_rx_mode();
    
    ESP_LOGI(TAG, "CRSF protocol initialized successfully");
    return ESP_OK;
}

esp_err_t crsf_send_frame(crsf_frame_t *frame) {
    if (!frame) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Calculate and set CRC
    frame->crc = crsf_calculate_crc(&frame->length, frame->length - 1);
    
    // Switch to TX mode for transmission
    crsf_set_tx_mode();
    
    // Small delay to ensure GPIO matrix has switched
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Send frame
    int bytes_written = uart_write_bytes(CRSF_UART_NUM, frame, frame->length + 1);
    if (bytes_written != frame->length + 1) {
        ESP_LOGE(TAG, "Failed to send complete frame");
        // Switch back to RX mode even on error
        crsf_set_rx_mode();
        return ESP_ERR_INVALID_RESPONSE;
    }
    
    // Wait for transmission to complete
    uart_wait_tx_done(CRSF_UART_NUM, pdMS_TO_TICKS(100));
    
    // Switch back to RX mode
    crsf_set_rx_mode();
    
    ESP_LOGD(TAG, "Sent CRSF frame: sync=0x%02X, len=%d, type=0x%02X", 
             frame->sync, frame->length, frame->type);
    
    return ESP_OK;
}

esp_err_t crsf_send_command(uint8_t destination, uint8_t command, uint8_t *payload, uint8_t payload_len) {
    if (payload_len > CRSF_PAYLOAD_SIZE_MAX - 3) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    crsf_frame_t frame;
    frame.sync = CRSF_SYNC_BYTE;
    frame.length = payload_len + 4; // type + dest + origin + command + payload + crc
    frame.type = CRSF_FRAMETYPE_COMMAND;
    
    frame.payload[0] = destination;
    frame.payload[1] = CRSF_ADDRESS_CRSF_TRANSMITTER; // origin
    frame.payload[2] = command;
    
    if (payload && payload_len > 0) {
        memcpy(&frame.payload[3], payload, payload_len);
    }
    
    return crsf_send_frame(&frame);
}

esp_err_t crsf_set_frequency(uint8_t mode) {
    ESP_LOGI(TAG, "Setting frequency mode: %d", mode);
    
    uint8_t payload[2] = {0x00, mode}; // Parameter ID and value
    return crsf_send_command(CRSF_ADDRESS_ELRS_LUA, CRSF_COMMAND_SUBCMD_GENERAL, payload, 2);
}

esp_err_t crsf_set_power(uint8_t power) {
    ESP_LOGI(TAG, "Setting power level: %d", power);
    
    uint8_t payload[2] = {0x04, power}; // Parameter ID for TX Power and value
    return crsf_send_command(CRSF_ADDRESS_ELRS_LUA, CRSF_COMMAND_SUBCMD_GENERAL, payload, 2);
}

esp_err_t crsf_enter_bind_mode(void) {
    ESP_LOGI(TAG, "Entering bind mode");
    
    uint8_t payload[1] = {0x00}; // Bind command payload
    return crsf_send_command(CRSF_ADDRESS_ELRS_LUA, CRSF_COMMAND_SUBCMD_BIND, payload, 1);
}

esp_err_t crsf_send_telemetry_request(void) {
    ESP_LOGD(TAG, "Sending telemetry request");
    
    // Send heartbeat frame to request telemetry
    crsf_frame_t frame;
    frame.sync = CRSF_SYNC_BYTE;
    frame.length = 3; // type + dest + origin + crc
    frame.type = CRSF_FRAMETYPE_HEARTBEAT;
    
    frame.payload[0] = CRSF_ADDRESS_CRSF_RECEIVER; // destination
    frame.payload[1] = CRSF_ADDRESS_CRSF_TRANSMITTER; // origin
    
    return crsf_send_frame(&frame);
}

esp_err_t crsf_process_received_frame(uint8_t *data, size_t len) {
    if (!data || len < 3) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Ensure we're in RX mode
    crsf_set_rx_mode();
    
    // Find sync byte
    for (size_t i = 0; i < len - 2; i++) {
        if (data[i] == CRSF_SYNC_BYTE) {
            crsf_frame_t *frame = (crsf_frame_t *)&data[i];
            
            // Check if we have enough data for the frame
            if (i + frame->length + 1 > len) {
                ESP_LOGD(TAG, "Incomplete frame received");
                return ESP_ERR_INVALID_SIZE;
            }
            
            // Validate frame
            if (!crsf_validate_frame(frame)) {
                ESP_LOGW(TAG, "Invalid CRSF frame received");
                continue;
            }
            
            ESP_LOGD(TAG, "Received CRSF frame: type=0x%02X, len=%d", frame->type, frame->length);
            
            // Process frame based on type
            switch (frame->type) {
                case CRSF_FRAMETYPE_LINK_STATISTICS:
                    if (frame->length >= sizeof(crsf_link_statistics_t) + 2) {
                        crsf_link_statistics_t *stats = (crsf_link_statistics_t *)&frame->payload[0];
                        
                        // Update telemetry data
                        if (xSemaphoreTake(telemetry_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                            latest_telemetry.rssi = stats->uplink_rssi_1;
                            latest_telemetry.lq = stats->uplink_lq;
                            latest_telemetry.snr = stats->uplink_snr;
                            latest_telemetry.power = stats->uplink_tx_power;
                            latest_telemetry.rfmode = stats->rf_mode;
                            xSemaphoreGive(telemetry_mutex);
                        }
                        
                        ESP_LOGD(TAG, "Link statistics: RSSI=%d, LQ=%d, SNR=%d", 
                                stats->uplink_rssi_1, stats->uplink_lq, stats->uplink_snr);
                    }
                    break;
                
                case CRSF_FRAMETYPE_DEVICE_INFO:
                    ESP_LOGI(TAG, "Device info frame received");
                    break;
                
                case CRSF_FRAMETYPE_HEARTBEAT:
                    ESP_LOGD(TAG, "Heartbeat frame received");
                    break;
                
                default:
                    ESP_LOGD(TAG, "Unknown frame type: 0x%02X", frame->type);
                    break;
            }
            
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NOT_FOUND;
}

void crsf_pack_rc_channels(uint16_t *channels, uint8_t *packed_data) {
    if (!channels || !packed_data) {
        return;
    }
    
    // Clear output buffer
    memset(packed_data, 0, CRSF_RC_CHANNEL_DATA_SIZE);
    
    // Pack 16 11-bit channels into 22 bytes
    // Each channel is 11 bits (0-2047), where 1500 is center
    for (int i = 0; i < CRSF_RC_CHANNEL_COUNT; i++) {
        // Clamp channel value to 11-bit range
        uint16_t channel_value = channels[i] & 0x7FF;
        
        // Calculate bit position in output buffer
        int bit_pos = i * CRSF_RC_CHANNEL_BITS;
        int byte_pos = bit_pos / 8;
        int bit_offset = bit_pos % 8;
        
        // Pack the 11-bit value into the buffer
        if (bit_offset <= 5) {
            // Value fits in current byte and next byte
            packed_data[byte_pos] |= (channel_value << bit_offset) & 0xFF;
            packed_data[byte_pos + 1] |= (channel_value >> (8 - bit_offset)) & 0xFF;
        } else {
            // Value spans three bytes
            packed_data[byte_pos] |= (channel_value << bit_offset) & 0xFF;
            packed_data[byte_pos + 1] |= (channel_value >> (8 - bit_offset)) & 0xFF;
            packed_data[byte_pos + 2] |= (channel_value >> (16 - bit_offset)) & 0xFF;
        }
    }
}

void crsf_unpack_rc_channels(uint8_t *packed_data, uint16_t *channels) {
    if (!packed_data || !channels) {
        return;
    }
    
    // Unpack 22 bytes into 16 11-bit channels
    for (int i = 0; i < CRSF_RC_CHANNEL_COUNT; i++) {
        // Calculate bit position in input buffer
        int bit_pos = i * CRSF_RC_CHANNEL_BITS;
        int byte_pos = bit_pos / 8;
        int bit_offset = bit_pos % 8;
        
        // Extract 11-bit value from buffer
        uint16_t channel_value = 0;
        
        if (bit_offset <= 5) {
            // Value spans two bytes
            channel_value = (packed_data[byte_pos] >> bit_offset) & 0x7FF;
            channel_value |= ((packed_data[byte_pos + 1] << (8 - bit_offset)) & 0x7FF);
        } else {
            // Value spans three bytes
            channel_value = (packed_data[byte_pos] >> bit_offset) & 0x7FF;
            channel_value |= ((packed_data[byte_pos + 1] << (8 - bit_offset)) & 0x7FF);
            channel_value |= ((packed_data[byte_pos + 2] << (16 - bit_offset)) & 0x7FF);
        }
        
        // Mask to 11 bits and store
        channels[i] = channel_value & 0x7FF;
    }
}

esp_err_t crsf_send_rc_channels(uint16_t *channels) {
    if (!channels) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t packed_data[CRSF_RC_CHANNEL_DATA_SIZE];
    crsf_pack_rc_channels(channels, packed_data);
    
    return crsf_send_rc_channels_packed(packed_data);
}

esp_err_t crsf_send_rc_channels_packed(uint8_t *packed_data) {
    if (!packed_data) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Sending RC channels data");
    
    crsf_frame_t frame;
    frame.sync = CRSF_SYNC_BYTE;
    frame.length = CRSF_RC_CHANNEL_DATA_SIZE + 2; // payload + type + crc
    frame.type = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;
    
    // Copy channel data to payload
    memcpy(frame.payload, packed_data, CRSF_RC_CHANNEL_DATA_SIZE);
    
    return crsf_send_frame(&frame);
}