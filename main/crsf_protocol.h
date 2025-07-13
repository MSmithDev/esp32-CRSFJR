#ifndef CRSF_PROTOCOL_H
#define CRSF_PROTOCOL_H

#include "crsf_bridge.h"

// CRSF Protocol Constants
#define CRSF_SYNC_BYTE                    0xC8
#define CRSF_FRAME_SIZE_MAX               64
#define CRSF_PAYLOAD_SIZE_MAX             60
#define CRSF_CRC_POLY                     0xD5

// CRSF Address Constants
#define CRSF_ADDRESS_BROADCAST            0x00
#define CRSF_ADDRESS_USB                  0x10
#define CRSF_ADDRESS_TBS_CORE_PNP_PRO     0x80
#define CRSF_ADDRESS_RESERVED1            0x8A
#define CRSF_ADDRESS_CURRENT_SENSOR       0xC0
#define CRSF_ADDRESS_GPS                  0xC2
#define CRSF_ADDRESS_TBS_BLACKBOX         0xC4
#define CRSF_ADDRESS_FLIGHT_CONTROLLER    0xC8
#define CRSF_ADDRESS_RESERVED2            0xCA
#define CRSF_ADDRESS_RACE_TAG             0xCC
#define CRSF_ADDRESS_RADIO_TRANSMITTER    0xEA
#define CRSF_ADDRESS_CRSF_RECEIVER        0xEC
#define CRSF_ADDRESS_CRSF_TRANSMITTER     0xEE
#define CRSF_ADDRESS_ELRS_LUA             0xEF

// CRSF Frame Types
#define CRSF_FRAMETYPE_GPS                0x02
#define CRSF_FRAMETYPE_VARIO              0x07
#define CRSF_FRAMETYPE_BATTERY_SENSOR     0x08
#define CRSF_FRAMETYPE_BARO_ALTITUDE      0x09
#define CRSF_FRAMETYPE_HEARTBEAT          0x0B
#define CRSF_FRAMETYPE_VIDEO_TRANSMITTER  0x0F
#define CRSF_FRAMETYPE_LINK_STATISTICS    0x14
#define CRSF_FRAMETYPE_OPENTX_SYNC        0x10
#define CRSF_FRAMETYPE_RADIO_ID           0x3A
#define CRSF_FRAMETYPE_RC_CHANNELS_PACKED 0x16
#define CRSF_FRAMETYPE_ATTITUDE           0x1E
#define CRSF_FRAMETYPE_FLIGHT_MODE        0x21
#define CRSF_FRAMETYPE_DEVICE_PING        0x28
#define CRSF_FRAMETYPE_DEVICE_INFO        0x29
#define CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY 0x2B
#define CRSF_FRAMETYPE_PARAMETER_READ     0x2C
#define CRSF_FRAMETYPE_PARAMETER_WRITE    0x2D
#define CRSF_FRAMETYPE_ELRS_STATUS        0x2E
#define CRSF_FRAMETYPE_COMMAND            0x32
#define CRSF_FRAMETYPE_RADIO_CHANNELS     0x17
#define CRSF_FRAMETYPE_SUBSET_CHANNELS    0x18
#define CRSF_FRAMETYPE_LINK_STATISTICS_RX 0x1C
#define CRSF_FRAMETYPE_LINK_STATISTICS_TX 0x1D
#define CRSF_FRAMETYPE_ARDUPILOT_RESP     0x80

// CRSF Command Sub-Types
#define CRSF_COMMAND_SUBCMD_GENERAL       0x0A
#define CRSF_COMMAND_SUBCMD_BIND          0x01

// CRSF Frame Structure
typedef struct {
    uint8_t sync;      // 0xC8
    uint8_t length;    // Length of frame (excluding sync byte)
    uint8_t type;      // Frame type
    uint8_t payload[CRSF_PAYLOAD_SIZE_MAX];
    uint8_t crc;       // CRC8
} __attribute__((packed)) crsf_frame_t;

// CRSF Link Statistics Structure
typedef struct {
    uint8_t uplink_rssi_1;
    uint8_t uplink_rssi_2;
    uint8_t uplink_lq;
    int8_t uplink_snr;
    uint8_t active_antenna;
    uint8_t rf_mode;
    uint8_t uplink_tx_power;
    uint8_t downlink_rssi;
    uint8_t downlink_lq;
    int8_t downlink_snr;
} __attribute__((packed)) crsf_link_statistics_t;

// CRSF Device Info Structure
typedef struct {
    uint8_t destination;
    uint8_t origin;
    char device_name[60];
    uint32_t serialno;
    uint32_t hardware_ver;
    uint32_t software_ver;
    uint8_t num_params;
    uint8_t param_version;
} __attribute__((packed)) crsf_device_info_t;

// Function prototypes
esp_err_t crsf_init(void);
esp_err_t crsf_set_frequency(uint8_t mode);
esp_err_t crsf_set_power(uint8_t power);
esp_err_t crsf_enter_bind_mode(void);
esp_err_t crsf_send_telemetry_request(void);
esp_err_t crsf_send_command(uint8_t destination, uint8_t command, uint8_t *payload, uint8_t payload_len);
esp_err_t crsf_send_frame(crsf_frame_t *frame);
esp_err_t crsf_process_received_frame(uint8_t *data, size_t len);
uint8_t crsf_calculate_crc(uint8_t *data, size_t len);
bool crsf_validate_frame(crsf_frame_t *frame);

#endif // CRSF_PROTOCOL_H