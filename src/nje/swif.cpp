#include <nje/swif.h>
#include <time.h>

Nje105::Nje105(NjeHwIf *iface) {
    port = iface;
}

void Nje105::reset() {
    port->send_utf_string("NJER");
}

void Nje105::set_message(nje_msg_kind_t kind, nje_msg_num_t num, nje_msg_t msg) {
    char datetime[9] = {0};
    mk_datetime(datetime, 9);
    char buffer[129];
    snprintf(buffer, 129, "]011%s%02d%s%c%c%s", 
        kind == MSG_NORMAL ? "A110" : (kind == MSG_EMERGENCY ? "B111" : "F120"),
        num,
        datetime,
        msg.attributes.color, msg.attributes.decor,
        msg.content);
    port->send_utf_string(buffer);
}

void Nje105::delete_message(nje_msg_kind_t kind, nje_msg_num_t num) {
    char buffer[64];
    snprintf(buffer, 64, "]011%s%02d", 
        kind == MSG_NORMAL ? "A210" : (kind == MSG_EMERGENCY ? "B211" : "F220"),
        num);
    port->send_utf_string(buffer);
}

void Nje105::mk_datetime(char * buffer, size_t len) {
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    strftime(buffer, len, "%m%d%H%M", tm_info);
}
