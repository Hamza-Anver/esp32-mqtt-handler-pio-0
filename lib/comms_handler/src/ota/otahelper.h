#ifndef OTA_HELPER_H
#define OTA_HELPER_H

#include "config/confighelper.h"
#include <HTTPClient.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

class OTAHelper
{
public:
    OTAHelper(ConfigHelper *config_helper = nullptr);
    bool OTARunning();
    String GetOTAUpdateStatus();
    void CallOTAInternetUpdateAsync(String url);
    static void UpdateOTAInternetURL(void *pvParameters);
    void UpdateOTAFileUpload();
    String _update_url;
    bool _update_success = false;

private:
    
    String _error_msg;
    ConfigHelper *_config_helper;
};

#endif // OTA_HELPER_H