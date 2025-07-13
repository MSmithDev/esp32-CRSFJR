# ESP32 CRSF Command Bridge

A serial bridge application for ESP32-S3 that enables communication between a host system and a RadioMaster Nomad RF module using the CRSF v3 protocol.

## Features

- **Host Interface**: UART1 at 115200 baud for command input and response output
- **CRSF Interface**: UART2 at 420000 baud with half-duplex communication to RadioMaster Nomad
- **Command Support**: SET_FREQ, SET_PWR, SET_BIND, GET_TELEM, INFO
- **Telemetry Relay**: Automatic parsing and relay of CRSF telemetry data
- **Error Handling**: Comprehensive error responses for invalid commands and parameters

## Hardware Requirements

- **Target**: ESP32-S3 development board
- **RF Module**: RadioMaster Nomad (ExpressLRS v3.5.6)
- **Connections**:
  - Host UART: TX=GPIO17, RX=GPIO18
  - CRSF UART: TX=GPIO4, RX=GPIO5 (shared line for half-duplex)

## Software Requirements

- ESP-IDF v5.1.6 or later
- CMake 3.16 or later

## Building

1. Set up ESP-IDF environment:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

2. Configure the project:
   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig
   ```

3. Build the project:
   ```bash
   idf.py build
   ```

4. Flash to device:
   ```bash
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Usage

### Commands

Send commands to the host UART (115200 baud) terminated with CR/LF:

- `SET_FREQ:<mode>` - Set frequency mode (0-4)
- `SET_PWR:<level>` - Set power level (0-7)
- `SET_BIND` - Enter binding mode
- `GET_TELEM` - Get latest telemetry data
- `INFO` - Get device information

### Responses

- `OK` - Command executed successfully
- `ERR:BAD_CMD` - Invalid command format
- `ERR:BAD_PARAM` - Invalid parameter value
- `ERR:CRSF_FAIL` - CRSF communication failure
- `TELEM:<data>` - Telemetry data response

### Example Telemetry Response

```
TELEM:RSSI:85,LQ:100,SNR:12,PWR:2,MODE:1,BAD:0,GOOD:1234
```

## Architecture

The application consists of three main tasks:

1. **Host UART Task**: Handles command parsing and response formatting
2. **CRSF UART Task**: Manages CRSF protocol communication and frame processing
3. **Telemetry Relay Task**: Maintains telemetry data and handles periodic updates

## CRSF Protocol

The implementation follows CRSF v3 specification with support for:

- Device control commands (frequency, power, binding)
- Link statistics telemetry
- Device information queries
- Half-duplex UART communication

## Development

### Project Structure

```
├── main/
│   ├── main.c              # Application entry point
│   ├── crsf_bridge.c       # Main bridge implementation
│   ├── crsf_bridge.h       # Bridge definitions
│   ├── serial_commands.c   # Command parsing and handling
│   ├── serial_commands.h   # Command definitions
│   ├── crsf_protocol.c     # CRSF protocol implementation
│   ├── crsf_protocol.h     # CRSF protocol definitions
│   └── CMakeLists.txt      # Component configuration
├── CMakeLists.txt          # Project configuration
├── sdkconfig.defaults      # Default configuration
└── README.md              # This file
```

### Key Functions

- `crsf_set_frequency(uint8_t mode)` - Set Nomad frequency mode
- `crsf_set_power(uint8_t power)` - Set Nomad power level
- `crsf_enter_bind_mode(void)` - Enter binding mode
- `crsf_send_telemetry_request(void)` - Request telemetry data
- `serial_command_handler(const char *cmd)` - Parse and handle host commands
- `telemetry_relay_task(void *param)` - Telemetry relay task

## License

This project is licensed under the MIT License - see the LICENSE file for details.