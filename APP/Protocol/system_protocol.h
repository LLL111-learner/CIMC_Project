#ifndef SYSTEM_PROTOCOL_H
#define SYSTEM_PROTOCOL_H

#include "bsp_system.h"

#define CIMC_FRAME_START            0xA5B6U
#define CIMC_FRAME_END              0xB6A5U
#define CIMC_PROTOCOL_VERSION       0x02U
#define CIMC_DEVICE_ID_BROADCAST    0xFFFFU
#define CIMC_DEVICE_ID_DEFAULT      0x0001U

#define CIMC_FRAME_TYPE_CMD         0x01U
#define CIMC_FRAME_TYPE_ACK         0x02U
#define CIMC_FRAME_TYPE_HEARTBEAT   0x05U
#define CIMC_FRAME_TYPE_ERROR       0xFFU

#define CIMC_CMD_HEARTBEAT          0x8888U
#define CIMC_CMD_BROADCAST_SEARCH   0xFFFFU
#define CIMC_CMD_ERROR              0xEEEEU

#define CIMC_CMD_SYS_REBOOT         0x0101U
#define CIMC_CMD_QUERY_FW_VERSION   0x0104U
#define CIMC_CMD_UPGRADE_REQUEST    0x0501U
#define CIMC_CMD_SET_TIME           0x0105U
#define CIMC_CMD_QUERY_TIME         0x0106U
#define CIMC_CMD_QUERY_DEVICE_ID    0x0111U
#define CIMC_CMD_QUERY_BAUDRATE     0x0112U
#define CIMC_CMD_SET_DEVICE_ID      0x01A1U
#define CIMC_CMD_SET_BAUDRATE       0x01A2U
#define CIMC_CMD_QUERY_CH0          0x0201U
#define CIMC_CMD_QUERY_CH1          0x0202U
#define CIMC_CMD_QUERY_CH2          0x0221U
#define CIMC_CMD_SET_CH0_RATIO      0x0241U
#define CIMC_CMD_SET_CH1_RATIO      0x0242U
#define CIMC_CMD_SET_REPORT_INTERVAL 0x0261U
#define CIMC_CMD_SET_DAC            0x0301U
#define CIMC_CMD_AUTO_REPORT_START  0x0302U
#define CIMC_CMD_AUTO_REPORT_STOP   0x0303U
#define CIMC_CMD_ENTER_SLEEP        0x03AAU
#define CIMC_CMD_READ_THRESHOLDS    0x0400U
#define CIMC_CMD_READ_CH0_THRESHOLD 0x0401U
#define CIMC_CMD_READ_CH1_THRESHOLD 0x0402U
#define CIMC_CMD_WRITE_CH0_THRESHOLD 0x0411U
#define CIMC_CMD_WRITE_CH1_THRESHOLD 0x0412U
#define CIMC_CMD_ALARM_ACTIVE      0x0601U
#define CIMC_CMD_ALARM_QUERY       0x0602U
#define CIMC_CMD_ALARM_CLEAR       0x0603U

#define CIMC_MAX_PAYLOAD_LEN        64U
#define CIMC_BINARY_FRAME_OVERHEAD  13U
#define CIMC_BINARY_FRAME_MIN_LEN   CIMC_BINARY_FRAME_OVERHEAD
#define CIMC_MAX_BINARY_FRAME_LEN   (CIMC_BINARY_FRAME_OVERHEAD + CIMC_MAX_PAYLOAD_LEN)
#define CIMC_MAX_ASCII_FRAME_LEN    (CIMC_MAX_BINARY_FRAME_LEN * 2U)

typedef struct {
    uint16_t device_id;
    uint8_t frame_type;
    uint16_t command;
    uint8_t length;
    uint8_t version;
    uint8_t payload[CIMC_MAX_PAYLOAD_LEN];
} cimc_frame_t;

void cimc_protocol_init(void);
void cimc_protocol_process_bytes(const uint8_t *data, uint16_t len);
void cimc_protocol_task(void);

uint16_t cimc_crc16_modbus(const uint8_t *data, uint16_t len);
bool cimc_protocol_send_frame(uint8_t frame_type, uint16_t command, const uint8_t *payload, uint8_t len);
bool cimc_protocol_send_ok(uint16_t command);
bool cimc_protocol_send_error(void);
bool cimc_protocol_send_heartbeat(void);

#endif /* CIMC_PROTOCOL_H */
