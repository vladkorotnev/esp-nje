#pragma once
#include <freertos/FreeRTOS.h>

// Output option for foo_controlserver:
// %artist% - %title%

void foo_client_begin();

bool foo_is_playing();
void foo_get_text(char *, size_t);
TickType_t foo_last_recv();
