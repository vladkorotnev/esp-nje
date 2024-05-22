#pragma once

// The only existing library pulls in too much code for this task
// (an IMAP client that manages my WiFi and Flash? no thank you very much)
// So roll my own...

#include <map>
#include <vector>
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef enum imap_state {
    DISCONNECT, // <- need to connect
    CONNECT, // <- need to send auth
    AUTHING, // <- wait for auth result
    SELECTING, // <- succeeded auth, select inbox
    CHECKING, // <- `SEARCH UNSEEN`
    IDLE, // <- Waiting for realtime notification
} imap_state_t;

typedef int imap_message_id_t;

typedef struct imap_message_info {
    const char * sender_name;
    const char * sender_mail;
    const char * subject;
} imap_message_info_t;

// info is present when message appeared, NULL when message disappeared
typedef void (*imap_notify_callback_t)(imap_message_id_t, const imap_message_info_t *);

class ImapNotifier {
public:
    ImapNotifier(const char * serv, int port, const char * user, const char * pass);

    void set_callback(imap_notify_callback_t);
    void poll();

private:
    std::vector<imap_message_id_t> download_queue = {};
    bool did_just_expunge;

    imap_notify_callback_t callback = nullptr;
    imap_state_t state = DISCONNECT;
    char server[64] = { 0 };
    char port[8] = { 0 };
    char username[64] = { 0 };
    char password[64] = { 0 };

    static const size_t RXBUF_SIZE = 256;
    unsigned char rxbuf[RXBUF_SIZE] = { 0 };
    size_t rxbuf_ptr = 0;

    bool idle_supported = false;

    TaskHandle_t hTask;

    mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_x509_crt cacert;
	mbedtls_ssl_config conf;
	mbedtls_net_context server_fd;
    int tlserror;

    void connect();
    void login();
    void deinit();
    void free_socket();
    void manual_check();
    void download_queued();
    void download_one(imap_message_id_t);

    void send(const char *);
    void wait_lines();
    void process_line(const char*);
};