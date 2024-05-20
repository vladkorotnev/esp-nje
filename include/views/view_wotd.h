#pragma once
#include <nje/attribute.h>
#include <nje/message_mgr.h>
#include <service/wordnik.h>
#include <esp32-hal-log.h>

class WotdView {
public:
    WotdView(MessageManager* m) {
        mgr = m;
        mid.number = m->reserve(mid.kind);
    }

    ~WotdView() {
        mgr->remove(mid);
    }

    void update() {
        TickType_t u = wotd_get_last_update();
        if(last_update != u) {
            last_update = u;
            char word[64];
            char def[128];
            if(wotd_get_current(word, def)) {
                def[sizeof(def) - 1] = 0;
                word[sizeof(word) - 1] = 0;

                char buffer[128] = {0};
                nje_msg_attrib_t track_attr = { COLOR_RED, SCROLL };
                snprintf(buffer, 128, "%s~%c%c~: %s", word, track_attr.color, track_attr.decor, def);
                ESP_LOGI(LOG_TAG, "Word of the day: %s", buffer);
                mgr->update(mid, { .attributes = { COLOR_GREEN, SCROLL }, .content = buffer });
            } else {
                mgr->hide(mid);
            }
        }
    }

private:
    const char * LOG_TAG = "WOTD_VIEW";
    MessageManager * mgr;
    TickType_t last_update = 0;
    nje_msg_identifier_t mid = { .kind = MSG_NORMAL };
};