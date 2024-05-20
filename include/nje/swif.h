#pragma once
#include "hwif.h"
#include "attribute.h"

class Nje105 {
public:
    Nje105(NjeHwIf*);

    void reset();
    void set_message(nje_msg_kind_t, nje_msg_num_t, nje_msg_t);
    void delete_message(nje_msg_kind_t, nje_msg_num_t);
private:
    void mk_datetime(char*, size_t);
    NjeHwIf * port;
};