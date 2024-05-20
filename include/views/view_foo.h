#pragma once
#include <nje/attribute.h>
#include <nje/message_mgr.h>
#include <service/foo_client.h>
#include <esp32-hal-log.h>

class FooView {
public:
    FooView(MessageManager* m) {
        mgr = m;
        mid.number = m->reserve(mid.kind);
    }

    ~FooView() {
        mgr->remove(mid);
    }

    void update() {
        if(last_foo != foo_last_recv()) {
            ESP_LOGI(LOG_TAG, "New foo state");
            last_foo = foo_last_recv();
            if(foo_is_playing()) {
                char buffer[128] = {0};
                char artist[64] = {0};
                char track[64] = {0};
                foo_get_artist(artist, 64);
                foo_get_title(track, 64);
                nje_msg_attrib_t track_attr = { COLOR_GREEN, SCROLL };
                snprintf(buffer, 128, "%s -~%c%c~ %s", artist, track_attr.color, track_attr.decor, track);
                ESP_LOGI(LOG_TAG, "New track: %s", buffer);
                mgr->update(mid, { .attributes = { COLOR_RED, SCROLL }, .content = buffer });
            } else {
                mgr->hide(mid);
            }
        }
    }

private:
    const char * LOG_TAG = "FOO_VIEW";
    MessageManager * mgr;
    TickType_t last_foo = 0;
    nje_msg_identifier_t mid = { .kind = MSG_NORMAL };
};