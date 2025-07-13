#include "serial_commands.h"
#include "crsf_protocol.h"

static const char *TAG = "SERIAL_COMMANDS";

esp_err_t parse_host_command(const char *input, host_command_t *cmd) {
    if (!input || !cmd) {
        return ESP_ERR_INVALID_ARG;
    }

    // Clear command structure
    memset(cmd, 0, sizeof(host_command_t));

    // Parse command type
    if (strncmp(input, "SET_FREQ:", 9) == 0) {
        cmd->type = CMD_SET_FREQ;
        strncpy(cmd->param, input + 9, sizeof(cmd->param) - 1);
        cmd->param_value = atoi(cmd->param);
    } else if (strncmp(input, "SET_PWR:", 8) == 0) {
        cmd->type = CMD_SET_PWR;
        strncpy(cmd->param, input + 8, sizeof(cmd->param) - 1);
        cmd->param_value = atoi(cmd->param);
    } else if (strcmp(input, "SET_BIND") == 0) {
        cmd->type = CMD_SET_BIND;
    } else if (strcmp(input, "GET_TELEM") == 0) {
        cmd->type = CMD_GET_TELEM;
    } else if (strcmp(input, "INFO") == 0) {
        cmd->type = CMD_INFO;
    } else {
        cmd->type = CMD_UNKNOWN;
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t format_response(response_type_t type, const char *data, char *output, size_t output_size) {
    if (!output || output_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (type) {
        case RESP_OK:
            snprintf(output, output_size, "OK\r\n");
            break;
        case RESP_ERR_BAD_CMD:
            snprintf(output, output_size, "ERR:BAD_CMD\r\n");
            break;
        case RESP_ERR_BAD_PARAM:
            snprintf(output, output_size, "ERR:BAD_PARAM\r\n");
            break;
        case RESP_ERR_CRSF_FAIL:
            snprintf(output, output_size, "ERR:CRSF_FAIL\r\n");
            break;
        case RESP_TELEM:
            if (data) {
                snprintf(output, output_size, "TELEM:%s\r\n", data);
            } else {
                snprintf(output, output_size, "TELEM:NO_DATA\r\n");
            }
            break;
        default:
            snprintf(output, output_size, "ERR:UNKNOWN\r\n");
            break;
    }

    return ESP_OK;
}

esp_err_t handle_set_freq_command(const char *param) {
    if (!param) {
        return ESP_ERR_INVALID_ARG;
    }

    int freq_mode = atoi(param);
    
    // Validate frequency mode (example ranges)
    if (freq_mode < 0 || freq_mode > 4) {
        ESP_LOGE(TAG, "Invalid frequency mode: %d", freq_mode);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting frequency mode to: %d", freq_mode);
    
    // Send CRSF command to set frequency
    esp_err_t ret = crsf_set_frequency(freq_mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set frequency mode");
        return ret;
    }

    return ESP_OK;
}

esp_err_t handle_set_pwr_command(const char *param) {
    if (!param) {
        return ESP_ERR_INVALID_ARG;
    }

    int power_level = atoi(param);
    
    // Validate power level (example ranges)
    if (power_level < 0 || power_level > 7) {
        ESP_LOGE(TAG, "Invalid power level: %d", power_level);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Setting power level to: %d", power_level);
    
    // Send CRSF command to set power
    esp_err_t ret = crsf_set_power(power_level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set power level");
        return ret;
    }

    return ESP_OK;
}

esp_err_t handle_set_bind_command(void) {
    ESP_LOGI(TAG, "Entering bind mode");
    
    // Send CRSF command to enter bind mode
    esp_err_t ret = crsf_enter_bind_mode();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter bind mode");
        return ret;
    }

    return ESP_OK;
}

esp_err_t handle_get_telem_command(char *response, size_t response_size) {
    if (!response || response_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Get latest telemetry data
    if (xSemaphoreTake(telemetry_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(response, response_size, 
                "RSSI:%d,LQ:%d,SNR:%d,PWR:%d,MODE:%d,BAD:%lu,GOOD:%lu",
                latest_telemetry.rssi,
                latest_telemetry.lq,
                latest_telemetry.snr,
                latest_telemetry.power,
                latest_telemetry.rfmode,
                latest_telemetry.bad_packets,
                latest_telemetry.good_packets);
        xSemaphoreGive(telemetry_mutex);
    } else {
        strncpy(response, "TIMEOUT", response_size - 1);
        response[response_size - 1] = '\0';
    }

    return ESP_OK;
}

esp_err_t handle_info_command(char *response, size_t response_size) {
    if (!response || response_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Get Nomad configuration info
    snprintf(response, response_size, 
            "DEVICE:RadioMaster_Nomad,PROTOCOL:CRSF_v3,ELRS:v3.5.6,BRIDGE:v1.0.0");

    return ESP_OK;
}