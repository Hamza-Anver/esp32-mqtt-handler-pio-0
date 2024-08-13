#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "esp_log.h"

class OTAUpdate {
public:
    OTAUpdate(const char* updateUrl);
    void performUpdate();

private:
    const char* updateUrl;
    static const char* TAG;
};

#endif
