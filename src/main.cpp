#include <Arduino.h>
#include <map>
#include "hw_config.h"
#include <network/netmgr.h>
#include <network/otafvu.h>
#include <service/time.h>
#include <service/prefs.h>
#include <service/weather.h>
#include <service/sercon.h>
#include <service/wordnik.h>
#include <service/foo_client.h>
#include <service/imap/client.h>
#include <network/admin_panel.h>
#include <utils.h>
#include <state.h>
#include <nje/hwif.h>
#include <nje/swif.h>
#include <nje/message_mgr.h>
#include <views/view_foo.h>
#include <views/view_weather.h>
#include <views/view_wotd.h>
#include <views/view_time.h>

static char LOG_TAG[] = "APL_MAIN";

static OTAFVUManager * ota;

static device_state_t current_state = STATE_BOOT;
const device_state_t startup_state = STATE_IDLE;

Nje105 * sw;
NjeHwIf * hw;
MessageManager * mgr;
nje_msg_identifier_t sys_msg = { .kind = MSG_EMERGENCY };

FooView * fooView;
WeatherView * weatherView;
WotdView * wotdView;
TimeView * timeView;

void change_state(device_state_t to) {
    switch(to) {
        case STATE_IDLE:
            break;
        case STATE_OTAFVU:
            if(mgr != nullptr && sys_msg.number != 0) {
                mgr->remove_all(MSG_NORMAL);
                mgr->remove_all(MSG_COMMERCIAL);
                mgr->update(sys_msg, { .attributes = { COLOR_RED, SCROLL_BLINK_INVERSE }, .content = "OTA FVU Receive" });
            }
            break;
        case STATE_EXCLUSIVE_DIRECT:
            if(mgr != nullptr) {
                mgr->remove_all(MSG_NORMAL);
                mgr->remove_all(MSG_COMMERCIAL);
                mgr->remove_all(MSG_EMERGENCY);
                ESP_LOGI(LOG_TAG, "Exclusive mode on: use USB UART to talk directly to the NJE-105");
            }
            break;
        default:
            break;
    }
    current_state = to;
}

void sercon_line_callback(const char * line) {
    if(strcmp(line, "AT+!EXCLUSIVE=1") == 0 && current_state == STATE_IDLE) {
        change_state(STATE_EXCLUSIVE_DIRECT);
    } else if(strcmp(line, "AT+!EXCLUSIVE=0") == 0 && current_state == STATE_EXCLUSIVE_DIRECT) {
        change_state(STATE_IDLE);
    } else if(strcmp(line, "AT+!REBOOT=1") == 0 && current_state != STATE_OTAFVU) {
        ESP.restart();
    } else {
        hw->send_utf_string(line);
    }
}

std::map<imap_message_id_t, nje_msg_identifier_t> mail_map = {};
std::vector<ImapNotifier*> mail_clients = {};

void mail_cb(imap_message_id_t id, const imap_message_info_t * info) {
    nje_msg_identifier_t mid = { .kind = MSG_NORMAL, .number = 0 };

    if(mail_map.count(id) == 1) {
        mid = mail_map[id];
    } else if(info != nullptr) {
        mid.number = mgr->reserve(mid.kind);
    }

    if(info != nullptr) {
        char buf[128] = { 0 };
        snprintf(buf, 127, "%s ~%c%c~%s", 
            (info->sender_name[0] == '?') ? info->sender_mail : info->sender_name,
            COLOR_RED, SCROLL,
            (info->subject == nullptr || info->subject[0] == '?') ? "(新着メール)" : info->subject);
        if(current_state == STATE_IDLE)
            mgr->update(mid, { .attributes = { COLOR_GREEN, SCROLL }, .content = buf });
        mail_map[id] = mid;
    } else if (mid.number != 0) {
        if(current_state == STATE_IDLE)
            mgr->remove(mid);
        mail_map.erase(id);
    }
}

void setup() {
    hw = new NjeHwIf(NJE_TX_PIN, NJE_PORT);
    sw = new Nje105(hw);

    sw->set_power_mode((nje_power_mode_t) prefs_get_int(PREFS_KEY_NJE_WORK_MODE));

    mgr = new MessageManager(sw);
    sys_msg.number = mgr->reserve(sys_msg.kind);

    ESP_LOGI(LOG_TAG, "WiFi init");
    NetworkManager::startup();
    while(!NetworkManager::is_up()) {
        delay(1000);
    }

    ESP_LOGI(LOG_TAG, "Network: %s", NetworkManager::network_name());
    ESP_LOGI(LOG_TAG, "IP: %s", NetworkManager::current_ip().c_str());
    ota = new OTAFVUManager();

    timekeeping_begin();
    admin_panel_prepare(sw);

    String mail_accounts = prefs_get_string(PREFS_KEY_GMAIL_ACCOUNTS);
    char * mail_cstr = (char*) malloc(mail_accounts.length() + 1);
    strcpy(mail_cstr, mail_accounts.c_str());
    char * mail_line = strtok(mail_cstr, ":");
    while(mail_line != NULL) {
        char * login = mail_line;
        mail_line = strtok(NULL, "\n");
        if(mail_line == NULL) {
            ESP_LOGE(LOG_TAG, "Bad mail line for login %s, abandon parsing", login);
            break;
        }
        char * pass = mail_line;
        ESP_LOGV(LOG_TAG, "login = [%s], pass = [%s]", login, pass);

        ImapNotifier * m = new ImapNotifier("imap.gmail.com", 993, login, pass);
        m->set_callback(mail_cb);
        mail_clients.push_back(m);

        mail_line = strtok(NULL, ":");
    }

    foo_client_begin();
    weather_start();
    wotd_start();

    vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

    fooView = new FooView(mgr);
    weatherView = new WeatherView(mgr);
    wotdView = new WotdView(mgr);
    timeView = new TimeView(mgr);

    serial_begin(SERCON_PORT);
    serial_set_line_callback(sercon_line_callback);

    change_state(startup_state);
}

void loop() {
    switch(current_state) {
        case STATE_IDLE:
            timeView->update();
            fooView->update();
            weatherView->update();
            wotdView->update();
            break;

        case STATE_OTAFVU:
        case STATE_EXCLUSIVE_DIRECT:
            break;
        default:
            ESP_LOGE(LOG_TAG, "Unknown state %i", current_state);
            break;
    }
}
