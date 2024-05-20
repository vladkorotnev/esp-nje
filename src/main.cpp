#include <Arduino.h>
#include "hw_config.h"
#include <network/netmgr.h>
#include <network/otafvu.h>
#include <service/time.h>
#include <service/prefs.h>
#include <service/weather.h>
#include <service/sercon.h>
#include <service/wordnik.h>
#include <network/admin_panel.h>
#include <utils.h>
#include <state.h>
#include <service/foo_client.h>
#include <nje/hwif.h>
#include <nje/swif.h>
#include <nje/message_mgr.h>
#include <views/view_foo.h>
#include <views/view_weather.h>
#include <views/view_wotd.h>

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
NjeHwIf * hw;
MessageManager * mgr;

FooView * fooView;
WeatherView * weatherView;
WotdView * wotdView;

void sercon_line_callback(const char * line) {
    hw->send_utf_string(line);
}

void setup() {
    ESP_LOGI(LOG_TAG, "WiFi init");
    NetworkManager::startup();
    while(!NetworkManager::is_up()) {
        delay(1000);
        ESP_LOGV(LOG_TAG, "Still waiting...");
    }

    ESP_LOGI(LOG_TAG, "Network: %s", NetworkManager::network_name());
    ESP_LOGI(LOG_TAG, "IP: %s", NetworkManager::current_ip().c_str());
    ota = new OTAFVUManager();


    timekeeping_begin();
    admin_panel_prepare();
    foo_client_begin();
    weather_start();
    wotd_start();

    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

    hw = new NjeHwIf(NJE_TX_PIN, NJE_PORT);
    sw = new Nje105(hw);
    mgr = new MessageManager(sw);

    fooView = new FooView(mgr);
    weatherView = new WeatherView(mgr);
    wotdView = new WotdView(mgr);

    serial_begin(SERCON_PORT);
    serial_set_line_callback(sercon_line_callback);

    change_state(startup_state);
}


void processing() {
    switch(current_state) {
        case STATE_IDLE:
            fooView->update();
            weatherView->update();
            wotdView->update();
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
