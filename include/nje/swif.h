#pragma once
#include "hwif.h"
#include "attribute.h"

typedef enum nje_power_mode {
    ALWAYS_ON = 0,
    AUTO_SWITCHING = 1, // how does this work? on hours seems to have no effect on it
    ENERGY_SAVING = 2
} nje_power_mode_t;

typedef enum nje_scroll_speed {
    SCROLL_SPEED_SLOW = 0,
    SCROLL_SPEED_NORMAL = 1,
    SCROLL_SPEED_FAST = 2
} nje_scroll_speed_t;

typedef enum nje_blink_speed {
    BLINK_SPEED_NEVER = 0,
    BLINK_SPEED_SLOW = 1,
    BLINK_SPEED_NORMAL = 2,
    BLINK_SPEED_FAST = 3
} nje_blink_speed_t;

class Nje105 {
public:
    Nje105(NjeHwIf*);

    void reset();
    void set_message(nje_msg_kind_t, nje_msg_num_t, nje_msg_t);
    void delete_message(nje_msg_kind_t, nje_msg_num_t);

    void set_power_mode(nje_power_mode_t);
    void set_on_hour(uint8_t);
    void set_off_hour(uint8_t);

    void set_scroll_speed(nje_scroll_speed_t);
    void set_blink_speed(nje_blink_speed_t);
    void set_pause_time(uint8_t);
private:
    void mk_datetime(char*, size_t);
    NjeHwIf * port;
};