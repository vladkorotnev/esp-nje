#pragma once
#include <nje/attribute.h>
#include <nje/message_mgr.h>
#include <service/weather.h>
#include <esp32-hal-log.h>

class WeatherView {
public:
    WeatherView(MessageManager* m) {
        mgr = m;
        mid.number = m->reserve(mid.kind);
    }

    ~WeatherView() {
        mgr->remove(mid);
    }

    void update() {
        current_weather_t w;
        if(weather_get_current(&w) && w.last_updated != last_weather.last_updated) {
            last_weather = w;
            char buffer[128] = {0};

            nje_msg_attrib_t sub_attr = { COLOR_RED, SCROLL };
            nje_msg_attrib_t main_attr = { COLOR_GREEN, SCROLL };

            snprintf(buffer, 128, 
            "%s, ~%c%c~%.01f\370C. ~%c%c~Wind ~%c%c~%.01f~%c%c~ m/s. Pressure ~%c%c~%i~%c%c~ hPa.", 
                w.description, 
                main_attr.color, main_attr.decor,
                kelvin_to(w.temperature_kelvin, CELSIUS),
                sub_attr.color, sub_attr.decor,
                main_attr.color, main_attr.decor,
                w.windspeed_mps,
                sub_attr.color, sub_attr.decor,
                main_attr.color, main_attr.decor,
                w.pressure_hpa,
                sub_attr.color, sub_attr.decor
                );
            mgr->update(mid, { .attributes = sub_attr, .content = buffer });
        }
    }

private:
    const char * LOG_TAG = "WEA_VIEW";
    MessageManager * mgr;
    current_weather_t last_weather = { 0 };
    nje_msg_identifier_t mid = { .kind = MSG_NORMAL };
};