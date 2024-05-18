#include <Arduino.h>
#include "hw_config.h"
#include <network/netmgr.h>
#include <network/otafvu.h>
#include <service/time.h>
#include <service/prefs.h>
#include <service/weather.h>
#include <network/admin_panel.h>
#include <utils.h>
#include <state.h>
#include <service/foo_client.h>
#include <nje/hwif.h>
#include <nje/swif.h>

static char LOG_TAG[] = "APL_MAIN";

static OTAFVUManager * ota;

static device_state_t current_state = STATE_BOOT;
const device_state_t startup_state = STATE_IDLE;


void change_state(device_state_t to) {
    if(to == STATE_OTAFVU) {
        current_state = STATE_OTAFVU;
        return; // all other things handled in the FVU process
    }

    switch(to) {
        case STATE_IDLE:
            break;
        case STATE_OTAFVU:
        default:
            break;
    }
    current_state = to;
}

Nje105 * sw;

void setup() {
    // Set up serial for logs
    Serial.begin(115200);

    ESP_LOGI(LOG_TAG, "WiFi init");
    NetworkManager::startup();
    while(!NetworkManager::is_up()) {
        delay(1000);
    }

    ESP_LOGI(LOG_TAG, "Network: %s", NetworkManager::network_name());
    ESP_LOGI(LOG_TAG, "IP: %s", NetworkManager::current_ip().c_str());
    ota = new OTAFVUManager();


    timekeeping_begin();
    admin_panel_prepare();
    foo_client_begin();
    weather_start();

    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

    NjeHwIf * hw = new NjeHwIf(NJE_TX_PIN, NJE_PORT);

    sw = new Nje105(hw);
    sw->set_message(MSG_NORMAL, 1, { .color = COLOR_RED, .decor = SCROLL_BLINK }, "fucks sake");

    change_state(startup_state);
}


TickType_t last_foo = 0;
current_weather_t last_weather = { 0 };

void processing() {
    switch(current_state) {
        case STATE_IDLE:
        {
            if(last_foo != foo_last_recv()) {
                ESP_LOGI(LOG_TAG, "New foo");
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
                    sw->set_message(MSG_NORMAL, 1, { COLOR_RED, SCROLL }, buffer);
                } else {
                    sw->delete_message(MSG_NORMAL, 1);
                }
            }

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
                sw->set_message(MSG_NORMAL, 2, sub_attr, buffer);
            }
        }
            break;

        case STATE_OTAFVU:
            break;
        default:
            ESP_LOGE(LOG_TAG, "Unknown state %i", current_state);
            break;
    }
}

void loop() {
    processing();
}
