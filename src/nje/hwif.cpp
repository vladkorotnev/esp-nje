#include <nje/hwif.h>
#include <esp_err.h>
#include <driver/uart.h>
#include <time.h>
#include <esp32-hal-log.h>
#include <cstring>
#include <cerrno>
#include <UTF8toSJIS.h>

static char LOG_TAG[] = "NJEHW";
const char* UTF8SJIS_file = "/Utf8Sjis.tbl"; // must exist in SPIFFS
UTF8toSJIS u8ts;

NjeHwIf::NjeHwIf(gpio_num_t tx_pin, uart_port_t port) {
    _serial = port;
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_set_line_inverse(port, UART_SIGNAL_TXD_INV));
}

NjeHwIf::~NjeHwIf() {
    uart_driver_delete(_serial);
}

void NjeHwIf::send_pkt(const char * packet, size_t len) {
    // NJE-105 data format:
    // \r\n [datetime][packet] \r\n
    // Where
    //      - datetime e.g. 08132145 for Aug(8) 13, 21:45
    //      - packet is SJIS encoded data up to 128 bytes
    ESP_LOGV(LOG_TAG, "Packet write %i", len);
    if(len > 128) {
        ESP_LOGE(LOG_TAG, "%i is too long!", len);
    }

    uart_write_bytes(_serial, "\r\n", 2);

    char buffer[9] = { 0 };
    mk_datetime(buffer, 9);
    ESP_LOGV(LOG_TAG, "Time: %s", buffer);
    uart_write_bytes(_serial, buffer, 8);

    uart_write_bytes(_serial, packet, len);

    uart_write_bytes(_serial, "\r\n", 2);
}

void NjeHwIf::send_utf_string(const char * utf) {
    ESP_LOGV(LOG_TAG, "UTF write: %s", utf);

    uint8_t dest_str[129] = { 0 };
    uint16_t out_size = 0;

    u8ts.UTF8_to_SJIS_str_cnv(UTF8SJIS_file, utf, dest_str, &out_size);

    ESP_LOG_BUFFER_HEXDUMP(LOG_TAG, dest_str, out_size, ESP_LOG_INFO);
    send_pkt((char*)dest_str, out_size);
}

void NjeHwIf::mk_datetime(char * buffer, size_t len) {
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    strftime(buffer, len, "%m%d%H%M", tm_info);
}