# CRSF Command Bridge Application Specification

**Target Hardware:** ESP32-S3 (ESP-IDF v5.1.6)
**Target RF Module:** RadioMaster Nomad (ExpressLRS v3.5.6)
**Protocol:** CRSF v3 (Crossfire Serial Protocol)

---

## 1. Overview

This application will operate as a serial bridge between a host system and a RadioMaster Nomad RF module. The host will send commands over UART, which the ESP32 will parse and forward as CRSF commands to the Nomad. Telemetry data from the Nomad will be relayed back to the host. The connection to the Nomad uses a **single-wire half-duplex UART** as per CRSF specification.

---

## 2. Features

### 2.1 Serial Host Interface

* UART Port 1 (default baudrate: 115200)

* Host-to-ESP command interface:

  * `SET_FREQ:<mode>` — Set Nomad Frequency Mode
  * `SET_PWR:<level>` — Set Nomad Power Output Level
  * `SET_BIND` — Put Nomad into Binding Mode
  * `GET_TELEM` — Request latest telemetry data
  * `INFO` — Return Nomad configuration (future extension)

* Response format (examples):

  * `OK`
  * `ERR:BAD_CMD`
  * `ERR:BAD_PARAM`
  * `TELEM:<data>`

### 2.2 CRSF Interface (Nomad)

* UART Port 2 (CRSF TX/RX shared line - Half Duplex)
* Communicates using standard CRSF packets:

  * Device Control Commands (Frequency, Power, Binding)
  * Telemetry Requests/Handling

### 2.3 Telemetry Relay

* ESP32 listens for telemetry packets from the Nomad
* Parses CRSF telemetry packets
* Sends parsed telemetry data back to the host in the `TELEM:` format

---

## 3. System Architecture

| Component            | Description                                           |
| -------------------- | ----------------------------------------------------- |
| **UART1 (Host)**     | Receives commands and sends telemetry to host         |
| **UART2 (CRSF)**     | Single-wire half-duplex CRSF communication with Nomad |
| **CRSF Handler**     | Formats and transmits CRSF packets                    |
| **Telemetry Parser** | Parses CRSF telemetry packets from Nomad              |
| **Command Parser**   | Parses host commands and dispatches actions           |

---

## 4. Key Functions

```c
void crsf_set_frequency(uint8_t mode);
void crsf_set_power(uint8_t power);
void crsf_enter_bind_mode(void);
void crsf_send_telemetry_request(void);
void serial_command_handler(const char *cmd);
void telemetry_relay_task(void *param);
```

---

## 5. Command Mapping

| Host Command  | CRSF Command   | Payload                  | Notes                                      |
| ------------- | -------------- | ------------------------ | ------------------------------------------ |
| `SET_FREQ:xx` | Device Control | Payload ID + Mode        | Mode IDs per ELRS spec                     |
| `SET_PWR:xx`  | Device Control | Payload ID + Power Level |                                            |
| `SET_BIND`    | Bind Request   | Bind payload             |                                            |
| `GET_TELEM`   | N/A            | N/A                      | Sends last received telemetry back to host |

---

## 6. Error Handling

* Invalid command: `ERR:BAD_CMD`
* Invalid parameters: `ERR:BAD_PARAM`
* CRSF communication failure: `ERR:CRSF_FAIL`

---

## 7. Future Enhancements

* Auto baudrate detection for host UART
* MAVLink or MSP command translation support
* Telemetry data filtering or JSON formatting
* Configurable UART settings via menuconfig

---

## 8. Notes

* **Nomad Interface:** The CRSF connection must handle half-duplex with appropriate UART settings or GPIO direction control.
* **Telemetry Timing:** Telemetry relay task must not block CRSF receive loop.

---

*End of Specification*
