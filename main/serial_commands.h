#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include "crsf_bridge.h"

// Command parsing function
esp_err_t parse_host_command(const char *input, host_command_t *cmd);

// Response formatting function
esp_err_t format_response(response_type_t type, const char *data, char *output, size_t output_size);

// Command handlers
esp_err_t handle_set_freq_command(const char *param);
esp_err_t handle_set_pwr_command(const char *param);
esp_err_t handle_set_bind_command(void);
esp_err_t handle_get_telem_command(char *response, size_t response_size);
esp_err_t handle_info_command(char *response, size_t response_size);

#endif // SERIAL_COMMANDS_H