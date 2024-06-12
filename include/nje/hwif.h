#pragma once
#include <hal/gpio_hal.h>
#include <hal/uart_hal.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <iconv.h>

class NjeHwIf {
public:
    NjeHwIf(gpio_num_t tx_pin, uart_port_t port);
    ~NjeHwIf();
    void send_pkt(const char*, size_t);
    void send_utf_string(const char*);

private:
    void mk_datetime(char*, size_t);
    uart_port_t _serial;
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
};