#pragma once

#include <hal/gpio_hal.h>
#include <driver/uart.h>
#include <nje/attribute.h>

const gpio_num_t NJE_TX_PIN = GPIO_NUM_18;
const uart_port_t NJE_PORT = UART_NUM_2;
const uart_port_t SERCON_PORT = UART_NUM_0;

const int NJE_SYS_MSG_ID = 99;
const nje_msg_kind_t NJE_SYS_MSG_KIND = MSG_EMERGENCY;
