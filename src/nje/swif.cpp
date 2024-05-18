#include <nje/swif.h>
#include <time.h>

Nje105::Nje105(NjeHwIf *iface) {
    port = iface;
}

void Nje105::reset() {
    port->send_utf_string("NJER");
}

void Nje105::set_message(nje_msg_kind_t kind, uint8_t num, nje_msg_attrib_t attr, const char * text) {
    char datetime[9] = {0};
    mk_datetime(datetime, 9);
    char buffer[128];
    snprintf(buffer, 128, "]011%s%02d%s%c%c%s", 
        kind == MSG_NORMAL ? "A110" : (kind == MSG_EMERGENCY ? "F120" : "B111"),
        num,
        datetime,
        attr.color, attr.decor,
        text);
    port->send_utf_string(buffer);
}

void Nje105::mk_datetime(char * buffer, size_t len) {
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    strftime(buffer, len, "%m%d%H%M", tm_info);
}