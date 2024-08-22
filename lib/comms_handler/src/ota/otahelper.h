#ifndef OTA_HELPER_H
#define OTA_HELPER_H

#include "config/confighelper.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <A76XX.h>

class OTAHelper
{
public:
    enum OTAMethod_t
    {
        OTA_WIFI,
        OTA_LTE
    };

    OTAHelper(A76XX *a76xx);

    bool CheckUpdateJSON(String server, OTAHelper::OTAMethod_t method = OTAHelper::OTA_WIFI);

    bool CheckOTAHasMessage();
    bool GetOTAMessage(String &message);

    void handleOTAChunks(String filename, size_t index, uint8_t *data, size_t len, bool final, size_t reqlen = 0);
    void CallOTAInternetUpdateAsync(String url);
    static void UpdateOTAInternetURL(void *pvParameters);
    void UpdateOTAFileUpload();

    String _server_json_url;
    String _update_url;
    int server_port;
    bool _update_success = false;

private:

    bool _update_running = false;
    JsonDocument _update_meta_json;
    A76XX *_a76xx = nullptr;
    A76XXHTTPClient *_a76xx_http = nullptr;
    HTTPClient *_wifi_http;
    String _error_msg;
    String _update_msg;
    ConfigHelper *_config_helper;
};

#endif // OTA_HELPER_H