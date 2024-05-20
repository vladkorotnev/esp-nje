#include <network/otafvu.h>
#include <state.h>
#include <utils.h>
#include <nje/swif.h>

static char LOG_TAG[] = "OTAFVU";

extern "C" void OtaFvuTaskFunction( void * pvParameter );

void OtaFvuTaskFunction( void * pvParameter )
{
    ESP_LOGV(LOG_TAG, "Running task");
    OTAFVUManager * that = static_cast<OTAFVUManager*> ( pvParameter );

    while(1) {
        that->task();
    }
}

OTAFVUManager::OTAFVUManager() {
    ESP_LOGI(LOG_TAG, "Initializing");

    ArduinoOTA.setHostname((String("nje-") + getChipId()).c_str());
#ifndef OTAFVU_PORT
#define OTAFVU_PORT 3232
#endif
    ArduinoOTA.setPort(OTAFVU_PORT);
#ifdef OTAFVU_PASSWORD_HASH
    ArduinoOTA.setPasswordHash(OTAFVU_PASSWORD_HASH);
#endif
    ArduinoOTA.setRebootOnSuccess(false);


    ArduinoOTA.onStart([this] { this->get_ready(); });
    ArduinoOTA.onProgress([this] (unsigned int got, unsigned int of) { this->on_progress(got, of); });
    ArduinoOTA.onError([this] (ota_error_t error) { this->on_error(error); });
    ArduinoOTA.onEnd([this] { this->shut_up_and_explode(); });

    ArduinoOTA.begin();

    ESP_LOGV(LOG_TAG, "Creating task");
    if(xTaskCreate(
        OtaFvuTaskFunction,
        "OTAFVU",
        4096,
        this,
        10,
        &hTask
    ) != pdPASS) {
        ESP_LOGE(LOG_TAG, "Task creation failed!");
    }
}

void OTAFVUManager::task() {
    ArduinoOTA.handle();
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void OTAFVUManager::get_ready() {
    ESP_LOGI(LOG_TAG, "Get ready");
    change_state(STATE_OTAFVU);
}

void OTAFVUManager::shut_up_and_explode() {
    ESP_LOGI(LOG_TAG, "Shut up and explode!");
    ESP.restart();
}

void OTAFVUManager::on_error(ota_error_t error) {
    ESP_LOGE(LOG_TAG, "OTA error %i!!", error);
}

void OTAFVUManager::on_progress(unsigned int progress, unsigned int total) {
    static int lastPercent = 0;
    unsigned int percent =  (progress / (total / 100));
    ESP_LOGV(LOG_TAG, "Recv: %u%% [%u / %u]", percent, progress, total);
}