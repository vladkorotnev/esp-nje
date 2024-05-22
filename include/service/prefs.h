#pragma once
#include <Arduino.h>

typedef const char * prefs_key_t;

static constexpr prefs_key_t PREFS_KEY_WIFI_SSID = "ssid";
static constexpr prefs_key_t PREFS_KEY_WIFI_PASS = "pass";

static constexpr prefs_key_t PREFS_KEY_TIMEZONE = "tk_tz";
static constexpr prefs_key_t PREFS_KEY_TIMESERVER = "tk_ntp_serv";
static constexpr prefs_key_t PREFS_KEY_TIME_SYNC_INTERVAL_SEC = "tk_intv_s";

static constexpr prefs_key_t PREFS_KEY_WEATHER_LAT = "w_lat";
static constexpr prefs_key_t PREFS_KEY_WEATHER_LON = "w_lon";
static constexpr prefs_key_t PREFS_KEY_WEATHER_APIKEY = "w_apikey";
static constexpr prefs_key_t PREFS_KEY_WEATHER_INTERVAL_MINUTES = "w_interval_m";

static constexpr prefs_key_t PREFS_KEY_WORDNIK_APIKEY = "wd_apikey";
static constexpr prefs_key_t PREFS_KEY_WORDNIK_INTERVAL_MINUTES = "wd_interval_m";

static constexpr prefs_key_t PREFS_KEY_NJE_WORK_MODE = "nje_wmode";

static constexpr prefs_key_t PREFS_KEY_FOOBAR_SERVER = "foo_svr";
static constexpr prefs_key_t PREFS_KEY_FOOBAR_PORT = "foo_prt";

static constexpr prefs_key_t PREFS_KEY_GMAIL_ACCOUNTS = "gmail_acc";

void prefs_force_save();

String prefs_get_string(prefs_key_t, String def = String());
void prefs_set_string(prefs_key_t, String);

int prefs_get_int(prefs_key_t);
void prefs_set_int(prefs_key_t, int);

bool prefs_get_bool(prefs_key_t);
void prefs_set_bool(prefs_key_t, bool);