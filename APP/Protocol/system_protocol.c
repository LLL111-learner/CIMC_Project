#include "system_protocol.h"


static uint16_t s_device_id = CIMC_DEVICE_ID_DEFAULT;
static uint8_t s_ascii_rx[CIMC_MAX_ASCII_FRAME_LEN];
static uint16_t s_ascii_rx_len;


static uint8_t hex_value(uint8_t ch)
{
    if((ch >= '0') && (ch <= '9')) {
        return (uint8_t)(ch - '0');
    }
    if((ch >= 'A') && (ch <= 'F')) {
        return (uint8_t)(ch - 'A' + 10U);
    }
    if((ch >= 'a') && (ch <= 'f')) {
        return (uint8_t)(ch - 'a' + 10U);
    }
    return 0xFFU;
}

static bool ascii_to_bytes(const uint8_t *ascii, uint16_t ascii_len, uint8_t *out, uint16_t *out_len)
{
    uint16_t i;
    uint8_t high;
    uint8_t low;

    if((ascii == NULL) || (out == NULL) || (out_len == NULL) || ((ascii_len & 1U) != 0U)) {
        return false;
    }

    *out_len = (uint16_t)(ascii_len / 2U);
    for(i = 0U; i < *out_len; i++) {
        high = hex_value(ascii[i * 2U]);
        low = hex_value(ascii[(i * 2U) + 1U]);
        if((high == 0xFFU) || (low == 0xFFU)) {
            return false;
        }
        out[i] = (uint8_t)((high << 4U) | low);
    }

    return true;
}

static uint8_t hex_char(uint8_t value)
{
    static const uint8_t table[] = "0123456789ABCDEF";
    return table[value & 0x0FU];
}

static uint16_t put_u16_be(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8U);
    buf[1] = (uint8_t)value;
    return 2U;
}

static uint16_t get_u16_be(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8U) | buf[1]);
}

static void put_u32_be_payload(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24U);
    buf[1] = (uint8_t)(value >> 16U);
    buf[2] = (uint8_t)(value >> 8U);
    buf[3] = (uint8_t)value;
}

static uint32_t get_u32_be_payload(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24U) |
           ((uint32_t)buf[1] << 16U) |
           ((uint32_t)buf[2] << 8U) |
           (uint32_t)buf[3];
}

static void bytes_to_ascii(const uint8_t *bytes, uint16_t byte_len, uint8_t *ascii)
{
    uint16_t i;

    for(i = 0U; i < byte_len; i++) {
        ascii[i * 2U] = hex_char((uint8_t)(bytes[i] >> 4U));
        ascii[(i * 2U) + 1U] = hex_char(bytes[i]);
    }
}

static bool ascii_buffer_ends_with_frame_end(void)
{
    if(s_ascii_rx_len < 4U) {
        return false;
    }

    return (s_ascii_rx[s_ascii_rx_len - 4U] == 'B') &&
           (s_ascii_rx[s_ascii_rx_len - 3U] == '6') &&
           (s_ascii_rx[s_ascii_rx_len - 2U] == 'A') &&
           (s_ascii_rx[s_ascii_rx_len - 1U] == '5');
}

uint16_t cimc_crc16_modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint8_t j;

    for(i = 0U; i < len; i++) {
        crc ^= data[i];
        for(j = 0U; j < 8U; j++) {
            if((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

void cimc_protocol_init(void)
{
    s_device_id = cimc_param_get()->device_id;
    s_ascii_rx_len = 0U;
}

bool cimc_protocol_send_frame(uint8_t frame_type, uint16_t command, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[CIMC_MAX_BINARY_FRAME_LEN];
    uint8_t ascii[CIMC_MAX_ASCII_FRAME_LEN];
    uint16_t pos = 0U;
    uint16_t crc;

    if((len > CIMC_MAX_PAYLOAD_LEN) || ((len > 0U) && (payload == NULL))) {
        return false;
    }

    pos += put_u16_be(&frame[pos], CIMC_FRAME_START);
    pos += put_u16_be(&frame[pos], s_device_id);
    frame[pos++] = frame_type;
    pos += put_u16_be(&frame[pos], command);
    frame[pos++] = len;
    frame[pos++] = CIMC_PROTOCOL_VERSION;
    if(len > 0U) {
        memcpy(&frame[pos], payload, len);
        pos = (uint16_t)(pos + len);
    }

    crc = cimc_crc16_modbus(frame, pos);
    pos += put_u16_be(&frame[pos], crc);
    pos += put_u16_be(&frame[pos], CIMC_FRAME_END);

    bytes_to_ascii(frame, pos, ascii);
    rs485_send_bytes(ascii, (uint16_t)(pos * 2U));
    return true;
}

bool cimc_protocol_send_ok(uint16_t command)
{
    uint8_t ok = 0xFFU;
    return cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, command, &ok, 1U);
}

bool cimc_protocol_send_error(void)
{
    return cimc_protocol_send_frame(CIMC_FRAME_TYPE_ERROR, CIMC_CMD_ERROR, NULL, 0U);
}

bool cimc_protocol_send_heartbeat(void)
{
    return cimc_protocol_send_frame(CIMC_FRAME_TYPE_HEARTBEAT, CIMC_CMD_HEARTBEAT, NULL, 0U);
}

static bool send_auto_report_frame(void)
{
    uint8_t payload[12];

    cimc_data_build_auto_report_payload(payload);
    return cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, CIMC_CMD_AUTO_REPORT_START, payload, 12U);
}

static void send_plain_string(const char *text)
{
    if(text == NULL) {
        return;
    }

    rs485_send_bytes((const uint8_t *)text, (uint16_t)strlen(text));
}

static bool parse_frame(const uint8_t *bin, uint16_t bin_len, cimc_frame_t *frame)
{
    uint16_t crc_rx;
    uint16_t crc_calc;
    uint16_t end_flag;

    if((bin == NULL) || (frame == NULL) || (bin_len < CIMC_BINARY_FRAME_MIN_LEN)) {
        return false;
    }

    if(get_u16_be(&bin[0]) != CIMC_FRAME_START) {
        return false;
    }

    frame->device_id = get_u16_be(&bin[2]);
    frame->frame_type = bin[4];
    frame->command = get_u16_be(&bin[5]);
    frame->length = bin[7];
    frame->version = bin[8];

    if(frame->length > CIMC_MAX_PAYLOAD_LEN) {
        return false;
    }

    if(bin_len != (uint16_t)(CIMC_BINARY_FRAME_OVERHEAD + frame->length)) {
        return false;
    }

    if(frame->version != CIMC_PROTOCOL_VERSION) {
        return false;
    }

    crc_rx = get_u16_be(&bin[9U + frame->length]);
    end_flag = get_u16_be(&bin[11U + frame->length]);
    if(end_flag != CIMC_FRAME_END) {
        return false;
    }

    crc_calc = cimc_crc16_modbus(bin, (uint16_t)(9U + frame->length));
    if(crc_calc != crc_rx) {
        return false;
    }

    if(frame->length > 0U) {
        memcpy(frame->payload, &bin[9], frame->length);
    }

    return true;
}

static void handle_frame(const cimc_frame_t *frame)
{
    uint8_t payload[12];
    uint32_t utc_seconds;
    uint16_t dac_raw;
    uint16_t new_device_id;
    uint8_t new_baud_code;
    float float_value;
    char alarm_text[CIMC_ALARM_QUERY_BUFFER_SIZE];

    if(frame == NULL) {
        return;
    }

    if((frame->device_id != s_device_id) && (frame->device_id != CIMC_DEVICE_ID_BROADCAST)) {
        return;
    }

    if(cimc_data_auto_report_is_running() &&
       ((frame->frame_type != CIMC_FRAME_TYPE_CMD) || (frame->command != CIMC_CMD_AUTO_REPORT_STOP))) {
        return;
    }

    if((frame->frame_type == CIMC_FRAME_TYPE_HEARTBEAT) && (frame->command == CIMC_CMD_BROADCAST_SEARCH)) {
        (void)cimc_protocol_send_heartbeat();
        return;
    }

    if(frame->frame_type != CIMC_FRAME_TYPE_CMD) {
        (void)cimc_protocol_send_error();
        return;
    }

    switch(frame->command) {
    case CIMC_CMD_SYS_REBOOT:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        oled_show_bootloader_status();
        (void)cimc_protocol_send_ok(frame->command);
        cimc_system_reboot();
        return;

    case CIMC_CMD_QUERY_FW_VERSION:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        payload[0] = CIMC_FW_VER_MAJOR;
        payload[1] = CIMC_FW_VER_MINOR;
        payload[2] = CIMC_FW_VER_PATCH;
        payload[3] = CIMC_FW_VER_BUILD;
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_UPGRADE_REQUEST:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_system_request_bootloader_upgrade()) {
            oled_show_bootloader_status();
            (void)cimc_protocol_send_ok(frame->command);
            delay_1ms(20U);
            cimc_system_reboot();
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_SET_TIME:
        if(frame->length != 4U) {
            (void)cimc_protocol_send_error();
            return;
        }
        utc_seconds = get_u32_be_payload(frame->payload);
        cimc_system_set_time(utc_seconds);
        (void)cimc_protocol_send_ok(frame->command);
        return;

    case CIMC_CMD_QUERY_TIME:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        put_u32_be_payload(payload, cimc_system_get_time());
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_QUERY_DEVICE_ID:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        payload[0] = (uint8_t)(s_device_id >> 8U);
        payload[1] = (uint8_t)s_device_id;
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 2U);
        return;

    case CIMC_CMD_SET_DEVICE_ID:
        if(frame->length != 2U) {
            (void)cimc_protocol_send_error();
            return;
        }
        new_device_id = (uint16_t)(((uint16_t)frame->payload[0] << 8U) | frame->payload[1]);
        if(cimc_param_update_device_id(new_device_id)) {
            (void)cimc_protocol_send_ok(frame->command);
            s_device_id = new_device_id;
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_QUERY_BAUDRATE:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        payload[0] = cimc_system_get_baud_code();
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 1U);
        return;

    case CIMC_CMD_SET_BAUDRATE:
        if(frame->length != 1U) {
            (void)cimc_protocol_send_error();
            return;
        }
        new_baud_code = frame->payload[0];
        if(cimc_param_update_baud_code(new_baud_code)) {
            (void)cimc_protocol_send_ok(frame->command);
            cimc_system_apply_baud_rate();
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_QUERY_CH0:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_float_to_be(cimc_data_get_ch0_value(), payload);
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_QUERY_CH1:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_float_to_be(cimc_data_get_ch1_value(), payload);
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_QUERY_CH2:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_float_to_be(cimc_data_get_ch2_value(), payload);
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_SET_CH0_RATIO:
        if(frame->length != 4U) {
            (void)cimc_protocol_send_error();
            return;
        }
        float_value = cimc_data_float_from_be(frame->payload);
        if(cimc_data_set_ch0_ratio(float_value)) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_SET_CH1_RATIO:
        if(frame->length != 4U) {
            (void)cimc_protocol_send_error();
            return;
        }
        float_value = cimc_data_float_from_be(frame->payload);
        if(cimc_data_set_ch1_ratio(float_value)) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_SET_REPORT_INTERVAL:
        if(frame->length != 1U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_data_set_report_interval_code(frame->payload[0])) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_SET_DAC:
        if(frame->length != 2U) {
            (void)cimc_protocol_send_error();
            return;
        }
        dac_raw = (uint16_t)(((uint16_t)frame->payload[0] << 8U) | frame->payload[1]);
        if(dac_raw > 4095U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_data_set_dac_raw(dac_raw)) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_AUTO_REPORT_START:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_auto_report_start();
        (void)send_auto_report_frame();
        return;

    case CIMC_CMD_AUTO_REPORT_STOP:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_auto_report_stop();
        (void)cimc_protocol_send_ok(frame->command);
        return;

    case CIMC_CMD_ENTER_SLEEP:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        (void)cimc_protocol_send_ok(frame->command);
        cimc_system_enter_sleep_and_report_wakeup();
        return;

    case CIMC_CMD_READ_THRESHOLDS:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_float_to_be(cimc_data_get_ch0_threshold(), &payload[0]);
        cimc_data_float_to_be(cimc_data_get_ch1_threshold(), &payload[4]);
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 8U);
        return;

    case CIMC_CMD_READ_CH0_THRESHOLD:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_float_to_be(cimc_data_get_ch0_threshold(), payload);
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_READ_CH1_THRESHOLD:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_data_float_to_be(cimc_data_get_ch1_threshold(), payload);
        (void)cimc_protocol_send_frame(CIMC_FRAME_TYPE_ACK, frame->command, payload, 4U);
        return;

    case CIMC_CMD_WRITE_CH0_THRESHOLD:
        if(frame->length != 4U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_data_set_ch0_threshold(cimc_data_float_from_be(frame->payload))) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_WRITE_CH1_THRESHOLD:
        if(frame->length != 4U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_data_set_ch1_threshold(cimc_data_float_from_be(frame->payload))) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_ALARM_ACTIVE:
        if(frame->length != 1U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_alarm_set_active_mode(frame->payload[0])) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    case CIMC_CMD_ALARM_QUERY:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        cimc_alarm_build_query_response(alarm_text, sizeof(alarm_text));
        send_plain_string(alarm_text);
        return;

    case CIMC_CMD_ALARM_CLEAR:
        if(frame->length != 0U) {
            (void)cimc_protocol_send_error();
            return;
        }
        if(cimc_alarm_clear_records()) {
            (void)cimc_protocol_send_ok(frame->command);
        } else {
            (void)cimc_protocol_send_error();
        }
        return;

    default:
        break;
    }

    (void)cimc_protocol_send_error();
}

void cimc_protocol_process_bytes(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    if((data == NULL) || (len == 0U)) {
        return;
    }

    for(i = 0U; i < len; i++) {
        if((data[i] == '\r') || (data[i] == '\n') || (data[i] == ' ')) {
            continue;
        }

        if(hex_value(data[i]) == 0xFFU) {
            s_ascii_rx_len = 0U;
            (void)cimc_protocol_send_error();
            continue;
        }

        if(s_ascii_rx_len >= CIMC_MAX_ASCII_FRAME_LEN) {
            s_ascii_rx_len = 0U;
            (void)cimc_protocol_send_error();
            continue;
        }

        s_ascii_rx[s_ascii_rx_len++] = data[i];
    }
}

void cimc_protocol_task(void)
{
    uint8_t bin[CIMC_MAX_BINARY_FRAME_LEN];
    uint8_t header_bin[9];
    uint16_t bin_len = 0U;
    uint16_t expected_ascii_len;
    uint8_t payload_len;
    cimc_frame_t frame;

    while(s_ascii_rx_len >= (CIMC_BINARY_FRAME_MIN_LEN * 2U)) {
        if(!ascii_to_bytes(s_ascii_rx, 18U, header_bin, &bin_len)) {
            s_ascii_rx_len = 0U;
            (void)cimc_protocol_send_error();
            return;
        }

        if(get_u16_be(&header_bin[0]) != CIMC_FRAME_START) {
            memmove(s_ascii_rx, &s_ascii_rx[2], (size_t)(s_ascii_rx_len - 2U));
            s_ascii_rx_len = (uint16_t)(s_ascii_rx_len - 2U);
            continue;
        }

        payload_len = header_bin[7];
        if(payload_len > CIMC_MAX_PAYLOAD_LEN) {
            s_ascii_rx_len = 0U;
            (void)cimc_protocol_send_error();
            return;
        }

        expected_ascii_len = (uint16_t)((CIMC_BINARY_FRAME_OVERHEAD + payload_len) * 2U);
        if(s_ascii_rx_len < expected_ascii_len) {
            if(ascii_buffer_ends_with_frame_end()) {
                s_ascii_rx_len = 0U;
                (void)cimc_protocol_send_error();
            }
            return;
        }

        if(!ascii_to_bytes(s_ascii_rx, expected_ascii_len, bin, &bin_len) ||
           !parse_frame(bin, bin_len, &frame)) {
            s_ascii_rx_len = 0U;
            (void)cimc_protocol_send_error();
            return;
        }

        memmove(s_ascii_rx, &s_ascii_rx[expected_ascii_len], (size_t)(s_ascii_rx_len - expected_ascii_len));
        s_ascii_rx_len = (uint16_t)(s_ascii_rx_len - expected_ascii_len);
        handle_frame(&frame);
    }

    if(cimc_data_auto_report_is_due()) {
        (void)send_auto_report_frame();
    }
}
