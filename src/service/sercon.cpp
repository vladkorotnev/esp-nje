#include <esp32-hal-log.h>
#include <esp32-hal-uart.h>
#include <service/sercon.h>
#include <cstring>

static char LOG_TAG[] = "SERCON";

const int BUF_SIZE = 1024;

static QueueHandle_t uart_queue;
static TaskHandle_t hTask;
static serial_line_callback_t callback = nullptr;
static bool running = true;
static uart_port_t port;

static char buffer[BUF_SIZE] = { 0 };
static size_t ptr = 0;

void sercon_task(void * pvParameters) {
    while(running) {
        int read = uart_read_bytes(port, &buffer[ptr], BUF_SIZE - ptr, pdMS_TO_TICKS(10));
        if(read > 0) {
            ptr += read;
            buffer[ptr] = 0;
            const char * current = buffer;
            char * newline = strchr(current, '\n');
            while(newline != nullptr) {
                *newline = 0;
                if(char * cr = strchr(current, '\r')) {
                    *cr = 0;
                }
                ESP_LOGI(LOG_TAG, "Line: %s", current);
                if(callback != nullptr) callback(current);
                current = newline + 1;
                newline = strchr(current, '\n');
                ptr = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void serial_begin(uart_port_t uart) {
    port = uart;

    // I was going to just use the Arduino serial here, but it conflicts with ESP logs and makes a crash or a deadlock
    // So we have to roll our own
    const int uart_buffer_size = (1024 * 2);

    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(port, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    if(xTaskCreate(
        sercon_task,
        "SERCON",
        4096,
        nullptr,
        10,
        &hTask
    ) != pdPASS) {
        ESP_LOGE(LOG_TAG, "Task creation failed!");
    }
}

void serial_set_line_callback(serial_line_callback_t cb) {
    callback = cb;
}