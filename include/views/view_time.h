#pragma once
#include <nje/attribute.h>
#include <nje/message_mgr.h>
#include <service/time.h>
#include <esp32-hal-log.h>

class TimeView {
public:
    TimeView(MessageManager* m) {
        mgr = m;
        mid.number = m->reserve(mid.kind);
    }

    ~TimeView() {
        mgr->remove(mid);
    }

    void update() {
        tk_time_of_day_t now_time = get_current_time_coarse();
        tk_date now_date = get_current_date();

        now_time.millisecond = 0; now_time.second = 0;

        if(memcmp(&now_time, &last_time, sizeof(tk_time_of_day_t)) == 0 && memcmp(&now_date, &last_date, sizeof(tk_date_t)) == 0) return;

        last_time = now_time;
        last_date = now_date;

        char buffer[64] = {0};
        nje_msg_attrib_t date_attr = { .color = COLOR_RED, .decor = STILL };
        nje_msg_attrib_t time_attr = { .color = COLOR_GREEN, .decor = STILL };

        snprintf(buffer, 64, "~%c%c~ %02d月 %02d日（%s）~%c%c~　%02d:%02d", 
            date_attr.color, date_attr.decor,
            now_date.month + 1, now_date.day, day_kanji[now_date.dayOfWeek], 
            time_attr.color, time_attr.decor,
            now_time.hour, now_time.minute
        );

        // the NJE-105 doesn't like decor = STILL in here, so we need to use markup and use generic values here
        mgr->update(mid, { .attributes = { .color = COLOR_RED, .decor = SCROLL }, buffer });

        if(now_date.month == 0 && now_date.day == 1) {
            if(sub_mid.number != 0) return;
            sub_mid.number = mgr->reserve(sub_mid.kind);
            mgr->update(sub_mid, { .attributes = {.color = COLOR_YELLOW, .decor = PULL_INVERSE}, .content = "Happy New Year!" });
        } else if (sub_mid.number != 0) {
            mgr->remove(sub_mid);
            sub_mid.number = 0;
        }
    }

private:
    const char * LOG_TAG = "TIME_VIEW";
    const char * day_kanji[7] = { "日", "月", "火", "水", "木", "金", "土" };
    MessageManager * mgr;
    tk_time_of_day_t last_time = { 0 };
    tk_date last_date = { 0 };
    nje_msg_identifier_t mid = { .kind = MSG_NORMAL };
    nje_msg_identifier_t sub_mid = { .kind = MSG_NORMAL };
};