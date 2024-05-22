#include "network/admin_panel.h"
#include <service/prefs.h>
#include <service/weather.h>
#include <service/wordnik.h>
#include <nje/swif.h>
#include <GyverPortal.h>
#include <Arduino.h>

static char LOG_TAG[] = "ADMIN";
static TaskHandle_t hTask = NULL;
static GyverPortal ui;
static Nje105 * nje;

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

static const char * LOCAL_KEY_NJE_START_HR = "local_nje_start_h";
static const char * LOCAL_KEY_NJE_STOP_HR = "local_nje_stop_h";
static const char * LOCAL_KEY_NJE_SCRL_SPEED = "local_nje_scrl_speed";
static const char * LOCAL_KEY_NJE_BLINK_SPEED = "local_nje_blink_speed";
static const char * LOCAL_KEY_NJE_PAUSE_SPEED = "local_nje_pause_speed";
static const char * LOCAL_KEY_NJE_RESET = "local_nje_reset";

void build() {
    GP.BUILD_BEGIN();
    GP.PAGE_TITLE("NJE Admin Panel");
    GP.THEME(GP_DARK);
    GP.JQ_SUPPORT();

    GP.TITLE("NJE Admin Panel");

    GP.SPOILER_BEGIN("NJE-105 (Managed Settings)", GP_BLUE);
        GP.LABEL("Power:");
        GP.SELECT(PREFS_KEY_NJE_WORK_MODE, "Always On,Scheduled,Energy Saver", prefs_get_int(PREFS_KEY_NJE_WORK_MODE));
    GP.SPOILER_END();
    GP.BREAK();

    GP.SPOILER_BEGIN("NJE-105 (One-shot Settings)", GP_BLUE);
        GP.SPAN("Those settings are not stored in ESP-NJE and only sent to the NJE-105 once. Make sure the NJE-105 is on and connected.");
        GP.HR();
        GP.LABEL("Schedule On Hour:"); GP.NUMBER(LOCAL_KEY_NJE_START_HR, "9");
        GP.LABEL("Schedule Off Hour:"); GP.NUMBER(LOCAL_KEY_NJE_STOP_HR, "18");
        GP.HR();
        GP.LABEL("Scroll speed:"); 
        GP.SELECT(LOCAL_KEY_NJE_SCRL_SPEED, "Slow,Normal,Fast");
        GP.BREAK();
        GP.LABEL("Blink speed:");
        GP.SELECT(LOCAL_KEY_NJE_BLINK_SPEED, "No blink,Slow,Normal,Fast");
        GP.BREAK();
        GP.LABEL("Pause length:");
        GP.NUMBER(LOCAL_KEY_NJE_PAUSE_SPEED, "0 - never pause");
        GP.HR();
        GP.BUTTON(LOCAL_KEY_NJE_RESET, "Reset NJE-105");
    GP.SPOILER_END();
    GP.BREAK();

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
        GP.SPAN("E.g. JST-9 or AST4ADT,M3.2.0,M11.1.0. See <a href=\"https://www.iana.org/time-zones\">IANA TZ DB</a> for reference.");
    GP.SPOILER_END();
    GP.BREAK();

    GP.SPOILER_BEGIN("Foobar2000", GP_BLUE);
        render_string("Control Server IP", PREFS_KEY_FOOBAR_SERVER);
        render_int("Control Server Port:", PREFS_KEY_FOOBAR_PORT);
        GP.SPAN("Please set the format in foo_controlserver to: %artist%|%title%, and main delimiter to: |");
    GP.SPOILER_END();
    GP.BREAK();

    GP.SPOILER_BEGIN("OpenWeatherMap", GP_BLUE);
        render_string("Latitude", PREFS_KEY_WEATHER_LAT);
        render_string("Longitude", PREFS_KEY_WEATHER_LON);
        render_string("API Key", PREFS_KEY_WEATHER_APIKEY, true);
        render_int("Update interval [m]:", PREFS_KEY_WEATHER_INTERVAL_MINUTES);

        current_weather_t weather;
        if(weather_get_current(&weather)) {
            GP.HR();
            GP.LABEL("Current weather:");
            GP.LABEL(String(kelvin_to(weather.temperature_kelvin, CELSIUS)) + " C");
            GP.LABEL(String(weather.humidity_percent) + " %");
            GP.LABEL(String(weather.pressure_hpa) + "hPa");
        }
    GP.SPOILER_END();

    GP.BREAK();
    
    GP.SPOILER_BEGIN("Wordnik", GP_BLUE);
        render_string("API Key", PREFS_KEY_WORDNIK_APIKEY, true);
        render_int("Update interval [m]:", PREFS_KEY_WORDNIK_INTERVAL_MINUTES);

        char wotd[32];
        char definition[256];
        if(wotd_get_current(wotd, 32, definition, 256)) {
            GP.HR();
            GP.LABEL("Today's word:");
            GP.LABEL(wotd);
            GP.BREAK();
            GP.SPAN(definition);
        }
    GP.SPOILER_END();
    GP.BREAK();

    GP.SPOILER_BEGIN("Gmail", GP_BLUE);
        GP.SPAN("Enter one account per line, in the format of user@gmail.com:password. Use password suitable for IMAP login, e.g. app-specific password.");
        GP.AREA(PREFS_KEY_GMAIL_ACCOUNTS, 4, prefs_get_string(PREFS_KEY_GMAIL_ACCOUNTS));
    GP.SPOILER_END();
    GP.BREAK();

    GP.HR();

    GP.BUTTON(reboot_btn);
    GP.BUILD_END();
}

void action() {
    if(ui.click()) {
        save_int(PREFS_KEY_NJE_WORK_MODE, 0, 2);

        save_string(PREFS_KEY_WIFI_SSID);
        save_string(PREFS_KEY_WIFI_PASS);
        save_string(PREFS_KEY_TIMESERVER);
        save_string(PREFS_KEY_TIMEZONE);
        save_int(PREFS_KEY_TIME_SYNC_INTERVAL_SEC, 600, 21600);

        save_string(PREFS_KEY_WEATHER_LAT);
        save_string(PREFS_KEY_WEATHER_LON);
        save_string(PREFS_KEY_WEATHER_APIKEY);
        save_int(PREFS_KEY_WEATHER_INTERVAL_MINUTES, 10, 3600);

        save_string(PREFS_KEY_WORDNIK_APIKEY);
        save_int(PREFS_KEY_WORDNIK_INTERVAL_MINUTES, 60, 3600);

        save_string(PREFS_KEY_FOOBAR_SERVER);
        save_int(PREFS_KEY_FOOBAR_PORT, 1000, 9999);

        save_string(PREFS_KEY_GMAIL_ACCOUNTS);

        int tmp_int = 0;
        if(ui.clickInt(PREFS_KEY_NJE_WORK_MODE, tmp_int)) {
            nje->set_power_mode((nje_power_mode_t) tmp_int);
        }
        if(ui.clickInt(LOCAL_KEY_NJE_START_HR, tmp_int)) {
            nje->set_on_hour(tmp_int);
        }
        if(ui.clickInt(LOCAL_KEY_NJE_STOP_HR, tmp_int)) {
            nje->set_off_hour(tmp_int);
        }
        if(ui.clickInt(LOCAL_KEY_NJE_SCRL_SPEED, tmp_int)) {
            nje->set_scroll_speed((nje_scroll_speed_t) tmp_int);
        }
        if(ui.clickInt(LOCAL_KEY_NJE_BLINK_SPEED, tmp_int)) {
            nje->set_blink_speed((nje_blink_speed_t) tmp_int);
        }
        if(ui.clickInt(LOCAL_KEY_NJE_PAUSE_SPEED, tmp_int)) {
            if(tmp_int < 0) tmp_int = 0;
            if(tmp_int > 10) tmp_int = 10;
            nje->set_pause_time(tmp_int);
        }
        if(ui.click(LOCAL_KEY_NJE_RESET)) {
            nje->reset();
        }

        if(ui.click(reboot_btn)) {
            prefs_force_save();
            ESP.restart();
        }
    }
}

void admin_panel_prepare(Nje105 * n) {
    nje = n;
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