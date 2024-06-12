#pragma once
#include <driver/uart.h>

#define SERCON_BAUD 115200

typedef void (*serial_line_callback_t)(const char *);

void serial_begin(uart_port_t);
void serial_set_line_callback(serial_line_callback_t);