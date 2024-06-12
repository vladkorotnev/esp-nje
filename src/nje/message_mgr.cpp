#include <nje/message_mgr.h>
#include <esp32-hal-log.h>

static char LOG_TAG[] = "MSMGR";

MessageManager::MessageManager(Nje105 * iface) {
    display = iface;
    reserved_nums_normal = nje_msg_id_list();
    reserved_nums_cm = nje_msg_id_list();
    reserved_nums_emergency = nje_msg_id_list();
}

MessageManager::~MessageManager() {
    remove_all(MSG_NORMAL);
    remove_all(MSG_COMMERCIAL);
    remove_all(MSG_EMERGENCY);
}

nje_msg_num_t MessageManager::find_next_free(const nje_msg_id_list * in) {
    return in->_Find_first() - 1;
}

nje_msg_id_list * MessageManager::list_of(nje_msg_kind_t kind) {
    nje_msg_id_list * list;

    switch(kind) {
        case MSG_NORMAL: list = &reserved_nums_normal; break;
        case MSG_COMMERCIAL: list = &reserved_nums_cm; break;
        case MSG_EMERGENCY: list = &reserved_nums_emergency; break;
    }

    return list;
}

nje_msg_num_t MessageManager::internal_id_to_hw_id(nje_msg_num_t id) {
    if(id == -1) return -1;
    return NJE_MAX_MSG_NUM - (id + NJE_MIN_MSG_NUM);
}

nje_msg_num_t MessageManager::reserve(nje_msg_kind_t kind) {
    nje_msg_id_list * list = list_of(kind);
    nje_msg_num_t next = find_next_free(list);

    if(next < 0) {
        ESP_LOGW(LOG_TAG, "Warning: ran out of messages for kind %i", kind);
    } else {
        list->set(next);
        ESP_LOGV(LOG_TAG, "Reserved message %i of kind %i", next, kind);
    }

    return next;
}

void MessageManager::update(nje_msg_identifier_t id, nje_msg_t message) {
    nje_msg_id_list * list = list_of(id.kind);
    if(!list->test(id.number)) {
        ESP_LOGW(LOG_TAG, "Warning: updating message %i:%i before it was allocated.", id.kind, id.number);
        list->set(id.number);
    }
    display->set_message(id.kind, internal_id_to_hw_id(id.number), message);
}

void MessageManager::hide(nje_msg_identifier_t id) {
    nje_msg_id_list * list = list_of(id.kind);
    if(!list->test(id.number)) {
        ESP_LOGW(LOG_TAG, "Warning: hiding message %i:%i before it was allocated.", id.kind, id.number);
    }
    display->delete_message(id.kind, internal_id_to_hw_id(id.number));
    list->reset(id.number);
}

void MessageManager::remove(nje_msg_identifier_t id) {
    nje_msg_id_list * list = list_of(id.kind);
    hide(id);
    list->reset(id.number);
}

void MessageManager::remove_all(nje_msg_kind_t kind) {
    nje_msg_id_list * list = list_of(kind);

    for(nje_msg_num_t i = 0; i < NJE_MSG_COUNT; i++) {
        if(list->test(i)) {
            remove({ .kind = kind, .number = i });
        }
    }
}