#pragma once
#include <ArduinoOTA.h>

/// @brief Manages the ArduinoOTA update service
class OTAFVUManager {
public:
    OTAFVUManager();
    ~OTAFVUManager();

    /// @brief Perform pending work of the service. Normally this is called by the internal task of the service and doesn't need to be called externally.
    void task();
private:

    /// @brief Prepare to install an OTA update
    void get_ready();
    void on_error(ota_error_t);
    /// @brief Finalize installing an OTA update
    void shut_up_and_explode();
    void on_progress(unsigned int, unsigned int);
    TaskHandle_t hTask;
};
