#include <nje/swif.h>
#include <time.h>

// Commands taken from:
// https://web.archive.org/web/20070705065638/http://www.hydra.cx/msgbd_code.html

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

void Nje105::set_power_mode(nje_power_mode_t mode) {
    char buffer[16];
    snprintf(buffer, 16, "NJES02%02d", mode);
    port->send_utf_string(buffer);
}

void Nje105::set_on_hour(uint8_t start) {
    char buffer[16];
    snprintf(buffer, 16, "NJES03%02d", start);
    port->send_utf_string(buffer);
}

void Nje105::set_off_hour(uint8_t stop) {
    char buffer[16];
    snprintf(buffer, 16, "NJES04%02d", stop);
    port->send_utf_string(buffer);
}

void Nje105::set_scroll_speed(nje_scroll_speed_t v) {
    char buffer[16];
    snprintf(buffer, 16, "NJES06%02d", v);
    port->send_utf_string(buffer);
}

void Nje105::set_blink_speed(nje_blink_speed_t v) {
    char buffer[16];
    snprintf(buffer, 16, "NJES07%02d", v);
    port->send_utf_string(buffer);
}

void Nje105::set_pause_time(uint8_t v) {
    if(v > 10) v = 10;
    char buffer[16];
    snprintf(buffer, 16, "NJES08%02d", v);
    port->send_utf_string(buffer);
}
