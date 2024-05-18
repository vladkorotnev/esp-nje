#pragma once
#include "hwif.h"
#include "attribute.h"

class Nje105 {
public:
    Nje105(NjeHwIf*);

    void reset();
    void set_message(nje_msg_kind_t, uint8_t num, nje_msg_attrib_t, const char * text);
    void delete_message(nje_msg_kind_t, uint8_t num);
private:
    void mk_datetime(char*, size_t);
    NjeHwIf * port;
};