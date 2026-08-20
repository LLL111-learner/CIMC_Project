#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "gd32f4xx.h"
#include "bl_core.h"
#include "bl_config.h"
#include "bl_flash_if.h"
#include "bl_param.h"
#include "bsp_system.h"
#include "usart_app.h"
#include "systick.h"

typedef void (*app_entry_t)(void);

#define BL_CIMC_FRAME_START        0xA5B6U
#define BL_CIMC_FRAME_END          0xB6A5U
#define BL_CIMC_PROTOCOL_VERSION   0x02U
#define BL_CIMC_DEVICE_ID_DEFAULT  0x0001U
#define BL_CIMC_DEVICE_ID_BROADCAST 0xFFFFU
#define BL_CIMC_BAUDRATE_DEFAULT   19200UL
#define BL_CIMC_BAUD_CODE_4800     0x11U
#define BL_CIMC_BAUD_CODE_9600     0x12U
#define BL_CIMC_BAUD_CODE_19200    0x13U
#define BL_CIMC_BAUD_CODE_115200   0x14U
#define BL_CIMC_TYPE_CMD           0x01U
#define BL_CIMC_TYPE_ACK           0x02U
#define BL_CIMC_TYPE_ERROR         0xFFU
#define BL_CIMC_CMD_PREPARE        0x0502U
#define BL_CIMC_CMD_EXECUTE        0x0503U
#define BL_CIMC_MAX_PAYLOAD_LEN    64U
#define BL_CIMC_FRAME_OVERHEAD     13U
#define BL_CIMC_MIN_BINARY_LEN     BL_CIMC_FRAME_OVERHEAD
#define BL_CIMC_MAX_BINARY_LEN     (BL_CIMC_FRAME_OVERHEAD + BL_CIMC_MAX_PAYLOAD_LEN)
#define BL_CIMC_MAX_ASCII_LEN      (BL_CIMC_MAX_BINARY_LEN * 2U)
#define BL_OTA_MAGIC               0x5AA5C33CUL
#define BL_OTA_MAGIC_SIZE          4UL
#define BL_OTA_RX_CHUNK_SIZE       256UL
#define BL_OTA_RAM_BUFFER_SIZE     0x00018000UL
#define BL_OTA_FIRST_BYTE_TIMEOUT  10000UL
#define BL_OTA_IDLE_TIMEOUT        200UL
#define BL_OTA_TOTAL_TIMEOUT       30000UL
#define BL_OTA_EXECUTE_WAIT_MS     12000UL
#define BL_OTA_RETRY_WAIT_MS       12000UL
#define BL_APP_PARAM_OFFSET        0x800UL
#define BL_APP_PARAM_ADDR          (BL_PARAM_START_ADDR + BL_APP_PARAM_OFFSET)
#define BL_APP_PARAM_MAGIC         0x43494D43UL
#define BL_APP_PARAM_VERSION       0x00010001UL
#define BL_APP_PARAM_TAIL_MAGIC    0x5041524DUL

typedef struct {
    uint16_t device_id;
    uint8_t frame_type;
    uint16_t command;
    uint8_t length;
    uint8_t version;
    uint8_t payload[BL_CIMC_MAX_PAYLOAD_LEN];
} bl_cimc_frame_t;

typedef struct {
    uint16_t device_id;
    uint8_t baud_code;
    uint8_t reserved;
    float ch0_ratio;
    float ch1_ratio;
    float ch0_threshold;
    float ch1_threshold;
} bl_app_param_values_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    bl_app_param_values_t values;
    uint32_t crc32;
    uint32_t tail_magic;
} bl_app_param_record_t;

typedef enum {
    BL_UPGRADE_FRAME_NONE = 0,
    BL_UPGRADE_FRAME_PREPARED,
    BL_UPGRADE_FRAME_PREPARE_FAILED,
    BL_UPGRADE_FRAME_EXECUTED
} bl_upgrade_result_t;


static uint8_t bl_page_cache[BL_PARAM_PAGE_SIZE];
static uint8_t bl_ota_ram_buffer[BL_OTA_RAM_BUFFER_SIZE];
static uint32_t bl_ota_image_size;
static bool bl_ota_image_ready;
static uint16_t bl_cimc_device_id = BL_CIMC_DEVICE_ID_DEFAULT;
static uint32_t bl_cimc_baudrate = BL_CIMC_BAUDRATE_DEFAULT;

static bool bl_is_app_vector_valid(uint32_t app_base);


static uint32_t bl_crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc;
    uint32_t i;
    uint32_t j;

    crc = 0xFFFFFFFFUL;
    for(i = 0UL; i < len; i++) {
        crc ^= data[i];
        for(j = 0UL; j < 8UL; j++) {
            if((crc & 1UL) != 0UL) {
                crc = (crc >> 1UL) ^ 0xEDB88320UL;
            } else {
                crc >>= 1UL;
            }
        }
    }

    return crc ^ 0xFFFFFFFFUL;
}

static uint32_t bl_param_calc_crc(const bl_param_t *param)
{
    return bl_crc32_calc((const uint8_t *)param, (uint32_t)offsetof(bl_param_t, param_crc32));
}

static uint32_t bl_app_param_calc_crc(const bl_app_param_record_t *record)
{
    return bl_crc32_calc((const uint8_t *)record, (uint32_t)offsetof(bl_app_param_record_t, crc32));
}

static bool bl_cimc_baud_code_to_value(uint8_t baud_code, uint32_t *baudrate)
{
    if(baudrate == NULL) {
        return false;
    }

    switch(baud_code) {
    case BL_CIMC_BAUD_CODE_4800:
        *baudrate = 4800UL;
        return true;
    case BL_CIMC_BAUD_CODE_9600:
        *baudrate = 9600UL;
        return true;
    case BL_CIMC_BAUD_CODE_19200:
        *baudrate = 19200UL;
        return true;
    case BL_CIMC_BAUD_CODE_115200:
        *baudrate = 115200UL;
        return true;
    default:
        return false;
    }
}

static bool bl_app_param_is_valid(const bl_app_param_record_t *record)
{
    uint32_t baudrate;

    if(record == NULL) {
        return false;
    }

    if((record->magic != BL_APP_PARAM_MAGIC) ||
       (record->version != BL_APP_PARAM_VERSION) ||
       (record->tail_magic != BL_APP_PARAM_TAIL_MAGIC)) {
        return false;
    }

    if(record->crc32 != bl_app_param_calc_crc(record)) {
        return false;
    }

    if((record->values.device_id == 0U) ||
       (record->values.device_id == BL_CIMC_DEVICE_ID_BROADCAST)) {
        return false;
    }

    return bl_cimc_baud_code_to_value(record->values.baud_code, &baudrate);
}

static void bl_load_cimc_comm_config(void)
{
    const bl_app_param_record_t *record = (const bl_app_param_record_t *)BL_APP_PARAM_ADDR;

    bl_cimc_device_id = BL_CIMC_DEVICE_ID_DEFAULT;
    bl_cimc_baudrate = BL_CIMC_BAUDRATE_DEFAULT;

    if(bl_app_param_is_valid(record)) {
        bl_cimc_device_id = record->values.device_id;
        (void)bl_cimc_baud_code_to_value(record->values.baud_code, &bl_cimc_baudrate);
    }
}

static bool bl_cimc_is_addressed_to_me(uint16_t device_id)
{
    return (device_id == bl_cimc_device_id) || (device_id == BL_CIMC_DEVICE_ID_BROADCAST);
}

static uint8_t bl_hex_value(uint8_t ch)
{
    if((ch >= (uint8_t)'0') && (ch <= (uint8_t)'9')) {
        return (uint8_t)(ch - (uint8_t)'0');
    }
    if((ch >= (uint8_t)'A') && (ch <= (uint8_t)'F')) {
        return (uint8_t)(ch - (uint8_t)'A' + 10U);
    }
    if((ch >= (uint8_t)'a') && (ch <= (uint8_t)'f')) {
        return (uint8_t)(ch - (uint8_t)'a' + 10U);
    }
    return 0xFFU;
}

static uint8_t bl_hex_char(uint8_t value)
{
    static const uint8_t table[] = "0123456789ABCDEF";
    return table[value & 0x0FU];
}

static uint16_t bl_get_u16_be(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[0] << 8U) | buf[1]);
}

static uint16_t bl_put_u16_be(uint8_t *buf, uint16_t value)
{
    buf[0] = (uint8_t)(value >> 8U);
    buf[1] = (uint8_t)value;
    return 2U;
}

static void bl_bytes_to_ascii(const uint8_t *bytes, uint16_t byte_len, uint8_t *ascii)
{
    uint16_t i;

    for(i = 0U; i < byte_len; i++) {
        ascii[i * 2U] = bl_hex_char((uint8_t)(bytes[i] >> 4U));
        ascii[(i * 2U) + 1U] = bl_hex_char(bytes[i]);
    }
}

static bool bl_ascii_to_bytes(const uint8_t *ascii, uint16_t ascii_len, uint8_t *out, uint16_t *out_len)
{
    uint16_t i;
    uint8_t high;
    uint8_t low;

    if((ascii == NULL) || (out == NULL) || (out_len == NULL) || ((ascii_len & 1U) != 0U)) {
        return false;
    }

    *out_len = (uint16_t)(ascii_len / 2U);
    for(i = 0U; i < *out_len; i++) {
        high = bl_hex_value(ascii[i * 2U]);
        low = bl_hex_value(ascii[(i * 2U) + 1U]);
        if((high == 0xFFU) || (low == 0xFFU)) {
            return false;
        }
        out[i] = (uint8_t)((high << 4U) | low);
    }

    return true;
}

static uint16_t bl_crc16_modbus(const uint8_t *data, uint16_t len)
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

static bool bl_parse_cimc_frame(const uint8_t *bin, uint16_t bin_len, bl_cimc_frame_t *frame)
{
    uint16_t crc_rx;
    uint16_t crc_calc;
    uint16_t end_flag;

    if((bin == NULL) || (frame == NULL) || (bin_len < BL_CIMC_MIN_BINARY_LEN)) {
        return false;
    }

    if(bl_get_u16_be(&bin[0]) != BL_CIMC_FRAME_START) {
        return false;
    }

    frame->device_id = bl_get_u16_be(&bin[2]);
    frame->frame_type = bin[4];
    frame->command = bl_get_u16_be(&bin[5]);
    frame->length = bin[7];
    frame->version = bin[8];

    if((frame->length > BL_CIMC_MAX_PAYLOAD_LEN) ||
       (bin_len != (uint16_t)(BL_CIMC_FRAME_OVERHEAD + frame->length)) ||
       (frame->version != BL_CIMC_PROTOCOL_VERSION)) {
        return false;
    }

    crc_rx = bl_get_u16_be(&bin[9U + frame->length]);
    end_flag = bl_get_u16_be(&bin[11U + frame->length]);
    if(end_flag != BL_CIMC_FRAME_END) {
        return false;
    }

    crc_calc = bl_crc16_modbus(bin, (uint16_t)(9U + frame->length));
    if(crc_calc != crc_rx) {
        return false;
    }

    if(frame->length > 0U) {
        memcpy(frame->payload, &bin[9], frame->length);
    }

    return true;
}

static bool bl_send_cimc_frame(uint8_t frame_type, uint16_t command, const uint8_t *payload, uint8_t len)
{
    uint8_t frame[BL_CIMC_MAX_BINARY_LEN];
    uint8_t ascii[BL_CIMC_MAX_ASCII_LEN];
    uint16_t pos = 0U;
    uint16_t crc;

    if((len > BL_CIMC_MAX_PAYLOAD_LEN) || ((len > 0U) && (payload == NULL))) {
        return false;
    }

    pos += bl_put_u16_be(&frame[pos], BL_CIMC_FRAME_START);
    pos += bl_put_u16_be(&frame[pos], bl_cimc_device_id);
    frame[pos++] = frame_type;
    pos += bl_put_u16_be(&frame[pos], command);
    frame[pos++] = len;
    frame[pos++] = BL_CIMC_PROTOCOL_VERSION;
    if(len > 0U) {
        memcpy(&frame[pos], payload, len);
        pos = (uint16_t)(pos + len);
    }

    crc = bl_crc16_modbus(frame, pos);
    pos += bl_put_u16_be(&frame[pos], crc);
    pos += bl_put_u16_be(&frame[pos], BL_CIMC_FRAME_END);

    bl_bytes_to_ascii(frame, pos, ascii);
    rs485_send_bytes(ascii, (uint16_t)(pos * 2U));
    return true;
}

static bool bl_send_cimc_ok(uint16_t command)
{
    uint8_t ok = 0xFFU;
    return bl_send_cimc_frame(BL_CIMC_TYPE_ACK, command, &ok, 1U);
}

static bool bl_send_cimc_error(uint16_t command)
{
    return bl_send_cimc_frame(BL_CIMC_TYPE_ERROR, command, NULL, 0U);
}

static bool bl_rs485_read_byte(uint8_t *value)
{
    uint32_t stat;

    if(value == NULL) {
        return false;
    }

    stat = USART_STAT0(RS232_RS485_USART);
    if((stat & (USART_STAT0_ORERR | USART_STAT0_FERR | USART_STAT0_NERR | USART_STAT0_PERR | USART_STAT0_IDLEF)) != 0UL) {
        (void)USART_DATA(RS232_RS485_USART);
    }

    stat = USART_STAT0(RS232_RS485_USART);
    if((stat & USART_STAT0_RBNE) == 0UL) {
        return false;
    }

    *value = (uint8_t)(USART_DATA(RS232_RS485_USART) & 0xFFU);
    return true;
}

static void bl_rs485_flush_rx(void)
{
    uint8_t discard;

    while(bl_rs485_read_byte(&discard)) {
    }
}

static void bl_rs485_prepare_polling_rx(void)
{
    usart_interrupt_disable(RS232_RS485_USART, USART_INT_IDLE);
    usart_dma_receive_config(RS232_RS485_USART, USART_RECEIVE_DMA_DISABLE);
    (void)USART_STAT0(RS232_RS485_USART);
    (void)USART_DATA(RS232_RS485_USART);
    bl_rs485_flush_rx();
}

static bool bl_wait_ascii_frame(uint32_t timeout_ms, bl_cimc_frame_t *frame)
{
    uint8_t ascii[BL_CIMC_MAX_ASCII_LEN];
    uint8_t bin[BL_CIMC_MAX_BINARY_LEN];
    uint8_t header_bin[9];
    uint16_t ascii_len = 0U;
    uint16_t bin_len = 0U;
    uint16_t expected_ascii_len = 0U;
    uint32_t start_tick = systick_millis();
    uint8_t ch;

    if(frame == NULL) {
        return false;
    }

    while((systick_millis() - start_tick) < timeout_ms) {
        if(!bl_rs485_read_byte(&ch)) {
            continue;
        }

        if((ch == (uint8_t)'\r') || (ch == (uint8_t)'\n') || (ch == (uint8_t)' ')) {
            continue;
        }

        if(bl_hex_value(ch) == 0xFFU) {
            ascii_len = 0U;
            continue;
        }

        if(ascii_len >= BL_CIMC_MAX_ASCII_LEN) {
            ascii_len = 0U;
            continue;
        }

        ascii[ascii_len++] = ch;

        while(ascii_len >= (BL_CIMC_MIN_BINARY_LEN * 2U)) {
            if(!bl_ascii_to_bytes(ascii, 18U, header_bin, &bin_len)) {
                ascii_len = 0U;
                break;
            }

            if(bl_get_u16_be(&header_bin[0]) != BL_CIMC_FRAME_START) {
                memmove(ascii, &ascii[2], (size_t)(ascii_len - 2U));
                ascii_len = (uint16_t)(ascii_len - 2U);
                continue;
            }

            if(header_bin[7] > BL_CIMC_MAX_PAYLOAD_LEN) {
                ascii_len = 0U;
                break;
            }

            expected_ascii_len = (uint16_t)((BL_CIMC_FRAME_OVERHEAD + header_bin[7]) * 2U);
            if(ascii_len < expected_ascii_len) {
                break;
            }

            if(bl_ascii_to_bytes(ascii, expected_ascii_len, bin, &bin_len) &&
               bl_parse_cimc_frame(bin, bin_len, frame)) {
                return true;
            }

            ascii_len = 0U;
            break;
        }
    }

    return false;
}

static bool bl_receive_ota_bin(uint32_t *image_size)
{
    uint8_t *rx_buf = bl_ota_ram_buffer;
    uint32_t total = 0UL;
    uint32_t first_tick = systick_millis();
    uint32_t start_tick = first_tick;
    uint32_t last_rx_tick = first_tick;
    uint32_t erase_size;
    bool got_any = false;
    uint8_t byte_value;
    uint32_t magic;

    if(image_size == NULL) {
        return false;
    }

    while((systick_millis() - start_tick) < BL_OTA_TOTAL_TIMEOUT) {
        if(bl_rs485_read_byte(&byte_value)) {
            if(!got_any &&
               ((byte_value == (uint8_t)'\r') ||
                (byte_value == (uint8_t)'\n') ||
                (byte_value == (uint8_t)' ') ||
                (byte_value == (uint8_t)'\t'))) {
                continue;
            }

            if(total >= BL_OTA_RAM_BUFFER_SIZE) {
                return false;
            }

            rx_buf[total] = byte_value;
            total++;
            got_any = true;
            last_rx_tick = systick_millis();
            continue;
        }

        if(!got_any) {
            if((systick_millis() - first_tick) >= BL_OTA_FIRST_BYTE_TIMEOUT) {
                return false;
            }
        } else if((systick_millis() - last_rx_tick) >= BL_OTA_IDLE_TIMEOUT) {
            break;
        }
    }

    if(total <= BL_OTA_MAGIC_SIZE) {
        return false;
    }

    magic = ((uint32_t)rx_buf[0] << 24U) |
            ((uint32_t)rx_buf[1] << 16U) |
            ((uint32_t)rx_buf[2] << 8U) |
            (uint32_t)rx_buf[3];

    if(magic != BL_OTA_MAGIC) {
        return false;
    }

    *image_size = total - BL_OTA_MAGIC_SIZE;
    if((*image_size == 0UL) || (*image_size > BL_APP1_SIZE)) {
        return false;
    }

    erase_size = (total + BL_FLASH_PAGE_SIZE - 1UL) & ~(BL_FLASH_PAGE_SIZE - 1UL);
    if(!bl_flash_erase(BL_STAGING_START_ADDR, erase_size)) {
        return false;
    }
    if(!bl_flash_program(BL_STAGING_START_ADDR, rx_buf, total)) {
        return false;
    }

    return bl_flash_compare_bytes(BL_STAGING_START_ADDR, rx_buf, total);
}

static bool bl_copy_staging_to_app(void)
{
    uint8_t buffer[BL_COPY_CHUNK_SIZE];
    uint32_t copied = 0UL;
    uint32_t left;
    uint32_t chunk_size;
    uint32_t erase_size;
    const uint8_t *src;

    if(!bl_ota_image_ready || (bl_ota_image_size == 0UL) || (bl_ota_image_size > BL_APP1_SIZE)) {
        return false;
    }

    erase_size = BL_APP1_SIZE;
    if(!bl_flash_erase(BL_APP1_START_ADDR, erase_size)) {
        return false;
    }

    while(copied < bl_ota_image_size) {
        left = bl_ota_image_size - copied;
        chunk_size = (left > BL_COPY_CHUNK_SIZE) ? BL_COPY_CHUNK_SIZE : left;
        src = (const uint8_t *)(BL_STAGING_START_ADDR + BL_OTA_MAGIC_SIZE + copied);
        memcpy(buffer, src, chunk_size);
        if(!bl_flash_program(BL_APP1_START_ADDR + copied, buffer, chunk_size)) {
            return false;
        }
        copied += chunk_size;
    }

    return bl_is_app_vector_valid(BL_APP1_START_ADDR) &&
           bl_flash_compare_flash(BL_APP1_START_ADDR,
                                  BL_STAGING_START_ADDR + BL_OTA_MAGIC_SIZE,
                                  bl_ota_image_size);
}

static bool bl_is_app_vector_valid(uint32_t app_base)
{
    uint32_t msp;
    uint32_t reset_handler;

    
    msp = *(volatile uint32_t *)app_base;
    reset_handler = *(volatile uint32_t *)(app_base + 4UL);

    if((msp & 0x2FFE0000UL) != 0x20000000UL) {
        return false;
    }

    if((reset_handler < BL_FLASH_BASE_ADDR) || (reset_handler > BL_FLASH_END_ADDR)) {
        return false;
    }

    return true;
}

static bool bl_is_param_valid(const bl_param_t *param)
{
    uint32_t crc_expect;

    
    if(param->magic != BL_PARAM_MAGIC) {
        return false;
    }
    if(param->tail_magic != BL_PARAM_TAIL_MAGIC) {
        return false;
    }
    if(param->version != BL_PARAM_VERSION) {
        return false;
    }
    if((param->app1_addr != BL_APP1_START_ADDR) || (param->staging_addr != BL_STAGING_START_ADDR)) {
        return false;
    }
    if((param->app_size > BL_APP1_SIZE) || (param->app_size > BL_STAGING_SIZE)) {
        return false;
    }

    crc_expect = bl_param_calc_crc(param);
    if(crc_expect != param->param_crc32) {
        return false;
    }

    return true;
}

static void bl_param_set_default(bl_param_t *param)
{
    memset(param, 0, sizeof(bl_param_t));
    param->magic = BL_PARAM_MAGIC;
    param->version = BL_PARAM_VERSION;
    param->update_flag = BL_UPDATE_FLAG_IDLE;
    param->app1_addr = BL_APP1_START_ADDR;
    param->staging_addr = BL_STAGING_START_ADDR;
    param->log_write_index = 0UL;
    param->tail_magic = BL_PARAM_TAIL_MAGIC;
    param->param_crc32 = bl_param_calc_crc(param);
}

static bool bl_commit_param_page(const bl_param_t *param, const bl_log_entry_t *log_entry, bool append_log)
{
    uint32_t log_index;
    uint32_t log_offset;
    const uint8_t *page_ptr;
    bl_param_t main_copy;
    bl_param_t backup_copy;

   
    memcpy(bl_page_cache, (const void *)BL_PARAM_PAGE_ADDR, BL_PARAM_PAGE_SIZE);

    memcpy(&main_copy, param, sizeof(bl_param_t));
    main_copy.param_crc32 = bl_param_calc_crc(&main_copy);
    memcpy(&backup_copy, &main_copy, sizeof(bl_param_t));

    memcpy(&bl_page_cache[BL_PARAM_MAIN_ADDR - BL_PARAM_PAGE_ADDR], &main_copy, sizeof(bl_param_t));
    memcpy(&bl_page_cache[BL_PARAM_BACKUP_ADDR - BL_PARAM_PAGE_ADDR], &backup_copy, sizeof(bl_param_t));

    if(append_log && (log_entry != NULL)) {
        log_index = main_copy.log_write_index % BL_LOG_ENTRY_COUNT;
        log_offset = (BL_LOG_ADDR - BL_PARAM_PAGE_ADDR) + (log_index * BL_LOG_ENTRY_SIZE);
        memcpy(&bl_page_cache[log_offset], log_entry, sizeof(bl_log_entry_t));
    }

    if(!bl_flash_erase(BL_PARAM_PAGE_ADDR, BL_PARAM_PAGE_SIZE)) {
        return false;
    }

    page_ptr = (const uint8_t *)bl_page_cache;
    return bl_flash_program(BL_PARAM_PAGE_ADDR, page_ptr, BL_PARAM_PAGE_SIZE);
}

bool bl_commit_param(bl_param_t *param)
{
    bl_param_t repaired;

    
    if(!bl_is_param_valid(param)) {
        bl_param_set_default(&repaired);
        repaired.update_flag = param->update_flag;
        repaired.app_size = param->app_size;
        repaired.app_crc32 = param->app_crc32;
        repaired.last_error = param->last_error;
        *param = repaired;
    }

    param->magic = BL_PARAM_MAGIC;
    param->version = BL_PARAM_VERSION;
    param->app1_addr = BL_APP1_START_ADDR;
    param->staging_addr = BL_STAGING_START_ADDR;
    param->tail_magic = BL_PARAM_TAIL_MAGIC;

    return bl_commit_param_page(param, NULL, false);
}

static void bl_log_prepare(bl_log_entry_t *entry, uint32_t seq, uint32_t event_id, uint32_t result,
                           uint32_t value0, uint32_t value1, uint32_t value2)
{
    memset(entry, 0, sizeof(bl_log_entry_t));
    entry->magic = BL_LOG_MAGIC;
    entry->seq = seq;
    entry->event_id = event_id;
    entry->result = result;
    entry->value0 = value0;
    entry->value1 = value1;
    entry->value2 = value2;
    entry->crc32 = bl_crc32_calc((const uint8_t *)entry, (uint32_t)offsetof(bl_log_entry_t, crc32));
}

void bl_log_dump_uart(void)
{
    uint32_t i;
    const bl_log_entry_t *entry;
    uint32_t crc_expect;
    uint32_t valid_count = 0UL;

    my_printf(DEBUG_USART, "BL log dump:\r\n");
    for(i = 0UL; i < BL_LOG_ENTRY_COUNT; i++) {
        entry = (const bl_log_entry_t *)(BL_LOG_ADDR + (i * BL_LOG_ENTRY_SIZE));
        if(entry->magic != BL_LOG_MAGIC) {
            continue;
        }

        crc_expect = bl_crc32_calc((const uint8_t *)entry, (uint32_t)offsetof(bl_log_entry_t, crc32));
        my_printf(DEBUG_USART,
                  "  [%02u] seq=%u event=%u result=%u v0=0x%08X v1=0x%08X v2=0x%08X crc=%s\r\n",
                  i,
                  entry->seq,
                  entry->event_id,
                  entry->result,
                  entry->value0,
                  entry->value1,
                  entry->value2,
                  (crc_expect == entry->crc32) ? "OK" : "BAD");
        valid_count++;
    }

    if(valid_count == 0UL) {
        my_printf(DEBUG_USART, "  <empty>\r\n");
    }
}

static bool bl_copy_staging_image_to_app(uint32_t app_size)
{
    uint8_t buffer[BL_COPY_CHUNK_SIZE];
    uint32_t copied = 0UL;
    uint32_t erase_size;
    uint32_t left;
    uint32_t chunk_size;
    const uint8_t *src;

    if((app_size == 0UL) || (app_size > BL_APP1_SIZE) || (app_size > BL_STAGING_SIZE)) {
        return false;
    }

    
    erase_size = (app_size + BL_FLASH_PAGE_SIZE - 1UL) & ~(BL_FLASH_PAGE_SIZE - 1UL);
    if(!bl_flash_erase(BL_APP1_START_ADDR, erase_size)) {
        return false;
    }

    while(copied < app_size) {
        left = app_size - copied;
        chunk_size = (left > BL_COPY_CHUNK_SIZE) ? BL_COPY_CHUNK_SIZE : left;
        src = (const uint8_t *)(BL_STAGING_START_ADDR + copied);

        memcpy(buffer, src, chunk_size);
        if(!bl_flash_program(BL_APP1_START_ADDR + copied, buffer, chunk_size)) {
            return false;
        }
        copied += chunk_size;
    }

    return true;
}

static uint32_t bl_crc32_flash(uint32_t start_addr, uint32_t size)
{
    return bl_crc32_calc((const uint8_t *)start_addr, size);
}

static void bl_jump_to_app(uint32_t app_base)
{
    app_entry_t app_entry;
    uint32_t app_reset_handler;
    uint32_t i;

    
    __disable_irq();

    SysTick->CTRL = 0UL;
    SysTick->LOAD = 0UL;
    SysTick->VAL = 0UL;

    for(i = 0UL; i < 8UL; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFUL;
        NVIC->ICPR[i] = 0xFFFFFFFFUL;
    }

    __DSB();
    __ISB();

    SCB->VTOR = app_base;
    __set_MSP(*(volatile uint32_t *)app_base);

    app_reset_handler = *(volatile uint32_t *)(app_base + 4UL);
    app_entry = (app_entry_t)app_reset_handler;

    __enable_irq();
    app_entry();
}

static void bl_send_plain_string(const uint8_t *text, uint16_t len)
{
    rs485_send_bytes(text, len);
}

static bl_upgrade_result_t bl_handle_upgrade_frame(const bl_cimc_frame_t *frame)
{
    uint32_t image_size = 0UL;
    bool ok;

    if(frame == NULL) {
        return BL_UPGRADE_FRAME_NONE;
    }

    if(!bl_cimc_is_addressed_to_me(frame->device_id) ||
       (frame->frame_type != BL_CIMC_TYPE_CMD) ||
       (frame->length != 0U) ||
       (frame->version != BL_CIMC_PROTOCOL_VERSION)) {
        return BL_UPGRADE_FRAME_NONE;
    }

    if(frame->command == BL_CIMC_CMD_PREPARE) {
        ok = bl_receive_ota_bin(&image_size);
        if(ok) {
            bl_ota_image_size = image_size;
            bl_ota_image_ready = true;
            (void)bl_send_cimc_ok(BL_CIMC_CMD_PREPARE);
        } else {
            bl_ota_image_size = 0UL;
            bl_ota_image_ready = false;
            (void)bl_send_cimc_error(BL_CIMC_CMD_PREPARE);
            return BL_UPGRADE_FRAME_PREPARE_FAILED;
        }
        return BL_UPGRADE_FRAME_PREPARED;
    }

    if(frame->command == BL_CIMC_CMD_EXECUTE) {
        if(!bl_ota_image_ready) {
            (void)bl_send_cimc_error(BL_CIMC_CMD_EXECUTE);
            return BL_UPGRADE_FRAME_EXECUTED;
        }

        (void)bl_send_cimc_ok(BL_CIMC_CMD_EXECUTE);
        if(bl_copy_staging_to_app()) {
            /* Avoid a second reset path that would re-enter the normal 5s silent boot. */
            delay_1ms(20U);
        }
        return BL_UPGRADE_FRAME_EXECUTED;
    }

    return BL_UPGRADE_FRAME_NONE;
}

static bool bl_wait_upgrade_command_window(void)
{
    static const uint8_t msg_system_init[] = "system init\r\n";
    static const uint8_t msg_app_version[] = "Application Version 2.0.1.0\r\n";
    static const uint8_t msg_enter[] = "using command to interrupt start Application\r\n";
    static const uint8_t msg_prefix[] = "wait for start Application(";
    static const uint8_t msg_suffix[] = "s)\xE2\x80\xA6\xE2\x80\xA6\r\n";
    uint32_t start_tick;
    uint32_t elapsed_sec;
    uint32_t last_printed_sec = 0xFFFFFFFFUL;
    uint8_t countdown;
    uint8_t text[48];
    uint16_t len;
    bl_cimc_frame_t frame;
    bl_upgrade_result_t result;

    bl_send_plain_string(msg_system_init, (uint16_t)(sizeof(msg_system_init) - 1U));
    bl_send_plain_string(msg_app_version, (uint16_t)(sizeof(msg_app_version) - 1U));
    bl_send_plain_string(msg_enter, (uint16_t)(sizeof(msg_enter) - 1U));
    bl_rs485_prepare_polling_rx();
    start_tick = systick_millis();

    while((systick_millis() - start_tick) < 10000UL) {
        elapsed_sec = (systick_millis() - start_tick) / 1000UL;
        if(elapsed_sec != last_printed_sec) {
            last_printed_sec = elapsed_sec;
            countdown = (uint8_t)(10UL - elapsed_sec);
            if(countdown >= 6U) {
                bl_send_plain_string(msg_system_init, (uint16_t)(sizeof(msg_system_init) - 1U));
                bl_send_plain_string(msg_app_version, (uint16_t)(sizeof(msg_app_version) - 1U));
                bl_send_plain_string(msg_enter, (uint16_t)(sizeof(msg_enter) - 1U));
            }
            len = 0U;
            memcpy(&text[len], msg_prefix, sizeof(msg_prefix) - 1U);
            len = (uint16_t)(len + sizeof(msg_prefix) - 1U);
            if(countdown >= 10U) {
                text[len++] = (uint8_t)'1';
                text[len++] = (uint8_t)'0';
            } else {
                text[len++] = (uint8_t)((uint8_t)'0' + countdown);
            }
            memcpy(&text[len], msg_suffix, sizeof(msg_suffix) - 1U);
            len = (uint16_t)(len + sizeof(msg_suffix) - 1U);
            bl_send_plain_string(text, len);
        }

        if(bl_wait_ascii_frame(100UL, &frame)) {
            if(!bl_cimc_is_addressed_to_me(frame.device_id)) {
                continue;
            }
            result = bl_handle_upgrade_frame(&frame);
            if(result == BL_UPGRADE_FRAME_EXECUTED) {
                return true;
            }
            if(result == BL_UPGRADE_FRAME_PREPARED) {
                break;
            }
            if(result == BL_UPGRADE_FRAME_PREPARE_FAILED) {
                start_tick = systick_millis();
                last_printed_sec = 0xFFFFFFFFUL;
                bl_rs485_prepare_polling_rx();
                (void)bl_flash_erase(BL_STAGING_START_ADDR, BL_STAGING_SIZE);
                continue;
            }
            (void)bl_send_cimc_error(frame.command);
        }
    }

    if(bl_ota_image_ready) {
        start_tick = systick_millis();
        while((systick_millis() - start_tick) < BL_OTA_EXECUTE_WAIT_MS) {
            if(bl_wait_ascii_frame(100UL, &frame)) {
                if(!bl_cimc_is_addressed_to_me(frame.device_id)) {
                    continue;
                }
                result = bl_handle_upgrade_frame(&frame);
                if(result == BL_UPGRADE_FRAME_EXECUTED) {
                    return true;
                }
                if(result == BL_UPGRADE_FRAME_PREPARED) {
                    start_tick = systick_millis();
                    continue;
                }
                if(result == BL_UPGRADE_FRAME_PREPARE_FAILED) {
                    bl_rs485_prepare_polling_rx();
                    (void)bl_flash_erase(BL_STAGING_START_ADDR, BL_STAGING_SIZE);
                    start_tick = systick_millis();
                    while((systick_millis() - start_tick) < BL_OTA_RETRY_WAIT_MS) {
                        if(bl_wait_ascii_frame(100UL, &frame)) {
                            if(!bl_cimc_is_addressed_to_me(frame.device_id)) {
                                continue;
                            }
                            result = bl_handle_upgrade_frame(&frame);
                            if(result == BL_UPGRADE_FRAME_EXECUTED) {
                                return true;
                            }
                            if(result == BL_UPGRADE_FRAME_PREPARED) {
                                start_tick = systick_millis();
                                break;
                            }
                            if(result == BL_UPGRADE_FRAME_NONE) {
                                (void)bl_send_cimc_error(frame.command);
                            }
                        }
                    }
                    continue;
                }
                if(result == BL_UPGRADE_FRAME_NONE) {
                    (void)bl_send_cimc_error(frame.command);
                }
            }
        }
    }

    return false;
}

void bootloader_run(void)
{
    bl_param_t main_param;
    bl_param_t backup_param;
    bl_param_t working_param;
    bl_log_entry_t log_entry;
    bool main_valid;
    bool backup_valid;
    bool need_repair = false;
    bool fast_upgrade_requested = false;
    bool update_ok;
    uint32_t app_crc;

    
    memcpy(&main_param, (const void *)BL_PARAM_MAIN_ADDR, sizeof(bl_param_t));
    memcpy(&backup_param, (const void *)BL_PARAM_BACKUP_ADDR, sizeof(bl_param_t));

    main_valid = bl_is_param_valid(&main_param);
    backup_valid = bl_is_param_valid(&backup_param);

    if(main_valid && backup_valid) {
        if(main_param.update_counter >= backup_param.update_counter) {
            working_param = main_param;
        } else {
            working_param = backup_param;
            need_repair = true;
        }
    } else if(main_valid) {
        working_param = main_param;
        need_repair = true;
    } else if(backup_valid) {
        working_param = backup_param;
        need_repair = true;
    } else {
        bl_param_set_default(&working_param);
        need_repair = true;
        working_param.last_error = BL_ERR_PARAM_INVALID;
    }

    if(need_repair) {
        
        bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                       BL_LOG_EVENT_PARAM_RECOVER, 1UL, main_valid ? 1UL : 0UL, backup_valid ? 1UL : 0UL,
                       working_param.last_error);
        working_param.log_write_index = (working_param.log_write_index + 1UL) % BL_LOG_ENTRY_COUNT;
        (void)bl_commit_param_page(&working_param, &log_entry, true);
    }

    bl_load_cimc_comm_config();
    bsp_usart1_init_with_baudrate(bl_cimc_baudrate);

    if(working_param.update_flag == BL_UPDATE_FLAG_FAST_UPGRADE) {
        fast_upgrade_requested = true;
        working_param.update_flag = BL_UPDATE_FLAG_IDLE;
        working_param.app_size = 0UL;
        working_param.app_crc32 = 0UL;
        working_param.last_error = BL_ERR_NONE;
        (void)bl_commit_param(&working_param);
        bl_ota_image_size = 0UL;
        bl_ota_image_ready = false;
        (void)bl_flash_erase(BL_STAGING_START_ADDR, BL_STAGING_SIZE);
        bl_wait_upgrade_command_window();
    }

    if(working_param.update_flag == BL_UPDATE_FLAG_PENDING) {
       
        if((working_param.app_size == 0UL) || (working_param.app_size > BL_STAGING_SIZE) ||
           !bl_is_app_vector_valid(BL_STAGING_START_ADDR)) {
            working_param.update_flag = BL_UPDATE_FLAG_FAILED;
            working_param.fail_counter++;
            working_param.last_error = BL_ERR_STAGING_INVALID;

            bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                           BL_LOG_EVENT_UPDATE_FAIL, 0UL, BL_ERR_STAGING_INVALID, working_param.app_size, 0UL);
            working_param.log_write_index = (working_param.log_write_index + 1UL) % BL_LOG_ENTRY_COUNT;
            (void)bl_commit_param_page(&working_param, &log_entry, true);
            NVIC_SystemReset();
        }

        app_crc = bl_crc32_flash(BL_STAGING_START_ADDR, working_param.app_size);
        if(app_crc != working_param.app_crc32) {
            working_param.update_flag = BL_UPDATE_FLAG_FAILED;
            working_param.fail_counter++;
            working_param.last_error = BL_ERR_STAGING_INVALID;

            bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                           BL_LOG_EVENT_UPDATE_FAIL, 0UL, BL_ERR_STAGING_INVALID,
                           working_param.app_crc32, app_crc);
            working_param.log_write_index = (working_param.log_write_index + 1UL) % BL_LOG_ENTRY_COUNT;
            (void)bl_commit_param_page(&working_param, &log_entry, true);
            NVIC_SystemReset();
        }

        update_ok = bl_copy_staging_image_to_app(working_param.app_size);
        if(update_ok) {
            app_crc = bl_crc32_flash(BL_APP1_START_ADDR, working_param.app_size);
            if(app_crc == working_param.app_crc32) {
                working_param.update_flag = BL_UPDATE_FLAG_IDLE;
                working_param.update_counter++;
                working_param.last_error = BL_ERR_NONE;

                bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                               BL_LOG_EVENT_UPDATE_OK, 1UL, working_param.app_size, working_param.app_crc32, app_crc);
            } else {
                working_param.update_flag = BL_UPDATE_FLAG_FAILED;
                working_param.fail_counter++;
                working_param.last_error = BL_ERR_COPY_FAILED;

                bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                               BL_LOG_EVENT_UPDATE_FAIL, 0UL, BL_ERR_COPY_FAILED,
                               working_param.app_crc32, app_crc);
            }
        } else {
            working_param.update_flag = BL_UPDATE_FLAG_FAILED;
            working_param.fail_counter++;
            working_param.last_error = BL_ERR_COPY_FAILED;

            bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                           BL_LOG_EVENT_UPDATE_FAIL, 0UL, BL_ERR_COPY_FAILED, 0UL, 0UL);
        }

        working_param.log_write_index = (working_param.log_write_index + 1UL) % BL_LOG_ENTRY_COUNT;
        (void)bl_commit_param_page(&working_param, &log_entry, true);
        NVIC_SystemReset();
    }

    if(bl_is_app_vector_valid(BL_APP1_START_ADDR)) {
        if(!fast_upgrade_requested) {
            delay_1ms(5000U);
        }
        bl_jump_to_app(BL_APP1_START_ADDR);
    }

    bl_log_dump_uart();
   
    working_param.last_error = BL_ERR_APP1_INVALID;
    bl_log_prepare(&log_entry, working_param.update_counter + working_param.fail_counter,
                   BL_LOG_EVENT_JUMP_FAIL, 0UL, BL_ERR_APP1_INVALID, BL_APP1_START_ADDR, 0UL);
    working_param.log_write_index = (working_param.log_write_index + 1UL) % BL_LOG_ENTRY_COUNT;
    (void)bl_commit_param_page(&working_param, &log_entry, true);

    while(1) {
    }
}
