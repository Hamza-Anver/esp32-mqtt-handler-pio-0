#ifndef OTA_HELPER_H
#define OTA_HELPER_H

#include "config/confighelper.h"
#include <HTTPClient.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>

class OTAHelper {
public:
    OTAHelper(ConfigHelper *config_helper = nullptr);
    void CallOTAInternetUpdateAsync(String url);
    static void UpdateOTAInternetURL(void *pvParameters);
    void UpdateOTAUpload();
    String _update_url;
private:
    ConfigHelper *_config_helper;
    
};

#endif // OTA_HELPER_H