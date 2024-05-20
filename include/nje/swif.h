#pragma once
#include "hwif.h"
#include "attribute.h"

typedef enum nje_power_mode {
    ALWAYS_ON = 0,
    AUTO_SWITCHING = 1, // how does this work? on hours seems to have no effect on it
    ENERGY_SAVING = 2
} nje_power_mode_t;

class Nje105 {
public:
    Nje105(NjeHwIf*);

    void reset();
    void set_message(nje_msg_kind_t, nje_msg_num_t, nje_msg_t);
    void delete_message(nje_msg_kind_t, nje_msg_num_t);

    void set_power_mode(nje_power_mode_t);
    void set_on_hours(uint8_t start, uint8_t stop);
private:
    void mk_datetime(char*, size_t);
    NjeHwIf * port;
};