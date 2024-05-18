#include <Arduino.h>
#include "hw_config.h"
#include <network/netmgr.h>
#include <network/otafvu.h>
#include <service/time.h>
#include <service/prefs.h>
#include <network/admin_panel.h>
#include <utils.h>
#include <state.h>
#include <service/foo_client.h>

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

    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

    change_state(startup_state);
}


void processing() {
    switch(current_state) {
        case STATE_IDLE:
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
