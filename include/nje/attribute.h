#pragma once
#include <WString.h>

typedef enum nje_msg_kind : uint8_t {
    MSG_NORMAL,
    MSG_EMERGENCY,
    MSG_COMMERCIAL
} nje_msg_kind_t;

typedef enum nje_msg_color {
    COLOR_GREEN = 'A',
    COLOR_RED = 'B',
    COLOR_YELLOW = 'C'
} nje_msg_color_t;

typedef enum nje_msg_decor {
    SCROLL = 'A',
    SCROLL_BLINK,
    SCROLL_INVERSE,
    SCROLL_BLINK_INVERSE,
    PULL = 'E',
    PULL_BLINK,
    PULL_INVERSE,
    PULL_BLINK_INVERSE,
    PAUSE = 'I',
    PAUSE_BLINK,
    PAUSE_INVERSE,
    PAUSE_BLINK_INVERSE,
    STILL = 'W',
    STILL_BLINK,
    STILL_INVERSE,
    STILL_BLINK_INVERSE
} nje_msg_decor_t;

typedef struct __attribute__((packed))  nje_msg_attrib {
    nje_msg_color_t color;
    nje_msg_decor_t decor;
} nje_msg_attrib_t;

typedef int8_t nje_msg_num_t;
const nje_msg_num_t NJE_MIN_MSG_NUM = 1;
const nje_msg_num_t NJE_MAX_MSG_NUM = 100;
const nje_msg_num_t NJE_MSG_COUNT = NJE_MAX_MSG_NUM - NJE_MIN_MSG_NUM;

typedef struct __attribute__((packed)) nje_msg_identifier {
    nje_msg_kind_t kind;
    nje_msg_num_t number;
} nje_msg_identifier_t;

typedef struct nje_msg {
    nje_msg_attrib_t attributes;
    const char * content;
} nje_msg_t;