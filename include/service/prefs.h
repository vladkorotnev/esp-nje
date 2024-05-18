#pragma once
#include <Arduino.h>

typedef const char * prefs_key_t;

static constexpr prefs_key_t PREFS_KEY_WIFI_SSID = "ssid";
static constexpr prefs_key_t PREFS_KEY_WIFI_PASS = "pass";

static constexpr prefs_key_t PREFS_KEY_TIMEZONE = "tk_tz";
static constexpr prefs_key_t PREFS_KEY_TIMESERVER = "tk_ntp_serv";
static constexpr prefs_key_t PREFS_KEY_TIME_SYNC_INTERVAL_SEC = "tk_intv_s";

void prefs_force_save();

String prefs_get_string(prefs_key_t, String def = String());
void prefs_set_string(prefs_key_t, String);

int prefs_get_int(prefs_key_t);
void prefs_set_int(prefs_key_t, int);

bool prefs_get_bool(prefs_key_t);
void prefs_set_bool(prefs_key_t, bool);