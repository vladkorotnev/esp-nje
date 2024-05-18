#include <service/foo_client.h>
#include <freertos/FreeRTOS.h>
#include <esp32-hal-log.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>

static char LOG_TAG[] = "FOO";

#define RECV_BUF_SIZE 1024
#define TEXT_BUF_SIZE 256
#define PORT 3333

static int sockfd;
static bool running = true;
static bool connected = false;
static struct sockaddr_in serv_addr;
static struct hostent * server;
static TaskHandle_t hTask;
static char recv_buf[RECV_BUF_SIZE];

static char text_buf[TEXT_BUF_SIZE];
static bool play_sts = false;
static SemaphoreHandle_t sts_semaphore;
static TickType_t last_recv = 0;

bool foo_is_playing() {
    if(!xSemaphoreTake(sts_semaphore, pdMS_TO_TICKS(1000))) {
        ESP_LOGE(LOG_TAG, "Semaphore timed out");
        return false;
    }

    bool sts = play_sts;

    xSemaphoreGive(sts_semaphore);

    return sts;
}

void foo_get_text(char * buf, size_t buf_size) {
    if(!xSemaphoreTake(sts_semaphore, pdMS_TO_TICKS(1000))) {
        ESP_LOGE(LOG_TAG, "Semaphore timed out");
        return;
    }

    strncpy(buf, text_buf, buf_size);

    xSemaphoreGive(sts_semaphore);
}

TickType_t foo_last_recv() {
    return last_recv;
}

#define check(expr) if (!(expr)) { ESP_LOGE(LOG_TAG, #expr); return; }

void enable_keepalive(int sock) {
    int yes = 1;
    check(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) != -1);

    int idle = 1;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) != -1);

    int interval = 5;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) != -1);

    int maxpkt = 10;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) != -1);
}

char * parse_field(char ** current) {
    char * old_current = *current;
    char * next = strchr(old_current, '|');
    if(next == nullptr) {
        ESP_LOGE(LOG_TAG, "Parse error: no next field");
        return nullptr;
    }
    next[0] = 0;

    *current = next + 1;
    ESP_LOGV(LOG_TAG, "Field: %s", old_current);
    return old_current;
}

void parse_line(char * line) {
    ESP_LOGV(LOG_TAG, "Line: %s", line);
    
    char * current = line;

    char * type_str = parse_field(&current);
    int type = atoi(type_str);
    ESP_LOGI(LOG_TAG, "Type = %i", type);
    if(type < 111 || type > 113) {
        return;
    }

    char * dummy = parse_field(&current);
    dummy = parse_field(&current);
    dummy = parse_field(&current);

    // Cannot use parse_field here in case the track name might contain the separator character
    char * txt = current;
    size_t txt_len = strlen(txt);
    txt[txt_len - 1] = 0;

    ESP_LOGI(LOG_TAG, "Track: %s", txt);

    if(!xSemaphoreTake(sts_semaphore, pdMS_TO_TICKS(1000))) {
        ESP_LOGE(LOG_TAG, "Semaphore timed out");
        return;
    }

    play_sts = (type == 111);
    strncpy(text_buf, txt, TEXT_BUF_SIZE);

    xSemaphoreGive(sts_semaphore);
}

void parse_block(char * block) {
    char * current = block;
    char * next = strstr(current, "\r\n");
    if(next == nullptr) {
        ESP_LOGE(LOG_TAG, "Malformed string (missing \\r\\n)");
    }
    while(next != nullptr) {
        next[0] = 0x0;

        parse_line(current);

        current = &next[2];
        if(*current == 0x0) return;
        next = strstr(current, "\r\n");
    }
}

void foo_task(void * pvParameters) {
    int r = 0;

    memset(recv_buf, 0, RECV_BUF_SIZE);
    memset(text_buf, 0, TEXT_BUF_SIZE);

    server = gethostbyname("192.168.1.106");
    if (server == NULL) {
        ESP_LOGE(LOG_TAG, "ERROR, no such host");
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr,
           server->h_length);
    serv_addr.sin_port = htons(PORT);

    while(running) {
        if(sockfd > 0) {
            close(sockfd);
            sockfd = -1;
        }
        sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd <= 0) {
            ESP_LOGE(LOG_TAG, "ERROR opening socket");
            return;
        }
        ESP_LOGI(LOG_TAG, "sockfd = %i", sockfd);

        if(r = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
            ESP_LOGI(LOG_TAG, "Error (%i, errno = %i) connecting, retry in 5s...", r, errno);
            vTaskDelay(pdMS_TO_TICKS(5000));
        } else {
            ESP_LOGI(LOG_TAG, "Connected");
            enable_keepalive(sockfd);
            connected = true;
            while(connected) {
                r = recv(sockfd, recv_buf, RECV_BUF_SIZE, 0);
                if(r < 0) {
                    ESP_LOGE(LOG_TAG, "Socket failure: %i", r);
                    connected = false;
                    last_recv = xTaskGetTickCount();
                    play_sts = false;
                    vTaskDelay(pdMS_TO_TICKS(5000));
                } else if (r > 0) {
                    recv_buf[r] = 0x0;
                    ESP_LOGV(LOG_TAG, "Receive: %s", recv_buf);
                    parse_block(recv_buf);
                }
            }
        }
    }
}

void foo_client_begin() {
    sts_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(sts_semaphore);

    if(xTaskCreate(
        foo_task,
        "FOOCLI",
        4096,
        nullptr,
        10,
        &hTask
    ) != pdPASS) {
        ESP_LOGE(LOG_TAG, "Task creation failed!");
    }
}