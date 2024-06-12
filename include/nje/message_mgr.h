#pragma once
#include "attribute.h"
#include "swif.h"
#include <bitset>

typedef std::bitset<NJE_MSG_COUNT> nje_msg_id_list;

class MessageManager {
public:
    MessageManager(Nje105*);
    ~MessageManager();

    nje_msg_num_t reserve(nje_msg_kind_t);
    void update(nje_msg_identifier, nje_msg_t);
    void hide(nje_msg_identifier);
    void remove(nje_msg_identifier_t);
    void remove_all(nje_msg_kind_t);

private:
    Nje105 * display;
    nje_msg_id_list reserved_nums_normal;
    nje_msg_id_list reserved_nums_cm;
    nje_msg_id_list reserved_nums_emergency;

    nje_msg_num_t find_next_free(const nje_msg_id_list *);
    nje_msg_num_t internal_id_to_hw_id(nje_msg_num_t);
    nje_msg_id_list * list_of(nje_msg_kind_t);
};