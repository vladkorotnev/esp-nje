#include "network/admin_panel.h"
#include <service/prefs.h>
#include <GyverPortal.h>
#include <Arduino.h>

static char LOG_TAG[] = "ADMIN";
static TaskHandle_t hTask = NULL;
static GyverPortal ui;

extern "C" void AdminTaskFunction( void * pvParameter );

void AdminTaskFunction( void * pvParameter )
{
    while(1) {
        ui.tick();
        vTaskDelay(100);
    }
}

GP_BUTTON reboot_btn("reboot", "Save & Restart NJE-OS");

static void render_bool(const char * title, prefs_key_t key) {
    GP.LABEL(title);
    GP.SWITCH(key, prefs_get_bool(key));
}

static void save_bool(prefs_key_t key) {
    bool temp = false;
    if(ui.clickBool(key, temp)) {
        prefs_set_bool(key, temp);
    }
}

static void render_int(const char * title, prefs_key_t key) {
    GP.LABEL(title);
    GP.NUMBER(key, "", prefs_get_int(key));
}

static void save_int(prefs_key_t key, int min, int max) {
    int temp;
    if(ui.clickInt(key, temp)) {
        ESP_LOGV(LOG_TAG, "Save int %s: min %i, max %i, val %i", key, min, max, temp);
        temp = std::min(temp, max);
        temp = std::max(temp, min);
        prefs_set_int(key, temp);
    }
}

static void render_string(const char * title, prefs_key_t key, bool is_pass = false) {
    GP.LABEL(title);
    if(is_pass) {
        GP.PASS_EYE(key, title, prefs_get_string(key));
    } else {
        GP.TEXT(key, title, prefs_get_string(key));
    }
}

static void save_string(prefs_key_t key) {
    String temp;
    if(ui.clickString(key, temp)) {
        prefs_set_string(key, temp);
    }
}

void build() {
    GP.BUILD_BEGIN();
    GP.PAGE_TITLE("NJE Admin Panel");
    GP.THEME(GP_DARK);
    GP.JQ_SUPPORT();

    GP.TITLE("NJE Admin Panel");

    GP.SPOILER_BEGIN("WiFi", GP_BLUE);
        render_string("SSID", PREFS_KEY_WIFI_SSID);
        render_string("Password", PREFS_KEY_WIFI_PASS, true);
    GP.SPOILER_END();
    GP.BREAK();

    GP.SPOILER_BEGIN("Time", GP_BLUE);
        render_string("NTP server:", PREFS_KEY_TIMESERVER);
        GP.BREAK();
        render_int("Sync interval [s]:", PREFS_KEY_TIME_SYNC_INTERVAL_SEC);
        GP.BREAK();
        render_string("Timezone descriptor:", PREFS_KEY_TIMEZONE);
        GP.LABEL("E.g. JST-9 or AST4ADT,M3.2.0,M11.1.0. See <a href=\"https://www.iana.org/time-zones\">IANA TZ DB</a> for reference.");
    GP.SPOILER_END();
    GP.BREAK();

    GP.HR();

    GP.BUTTON(reboot_btn);
    GP.BUILD_END();
}

void action() {
    if(ui.click()) {
        save_string(PREFS_KEY_WIFI_SSID);
        save_string(PREFS_KEY_WIFI_PASS);
        save_string(PREFS_KEY_TIMESERVER);
        save_string(PREFS_KEY_TIMEZONE);
        save_int(PREFS_KEY_TIME_SYNC_INTERVAL_SEC, 600, 21600);

        if(ui.click(reboot_btn)) {
            prefs_force_save();
            ESP.restart();
        }
    }
}

void admin_panel_prepare() {
    ui.attachBuild(build);
    ui.attach(action);
#if defined(ADMIN_LOGIN) && defined(ADMIN_PASS)
    ui.enableAuth(ADMIN_LOGIN, ADMIN_PASS);
#endif
    ui.start();

    ESP_LOGV(LOG_TAG, "Creating task");
    if(xTaskCreate(
        AdminTaskFunction,
        "ADM",
        4096,
        nullptr,
        10,
        &hTask
    ) != pdPASS) {
        ESP_LOGE(LOG_TAG, "Task creation failed!");
    }
}