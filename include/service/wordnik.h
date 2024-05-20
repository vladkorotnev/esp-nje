#pragma once
#include <freertos/FreeRTOS.h>

void wotd_start();
bool wotd_get_current(char * word, char * definition);
TickType_t wotd_get_last_update();