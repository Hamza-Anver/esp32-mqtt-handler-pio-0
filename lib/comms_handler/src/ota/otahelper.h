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
    void ClearOTAMessageStrings();

    bool CheckJSONForNewVersion(bool str_msg = true);

    void handleOTAChunks(String filename, size_t index, uint8_t *data, size_t len, bool final, size_t reqlen = 0, bool str_msg = true);
    void CallOTAInternetUpdateAsync(String url = "", bool autorestart = false, bool str_msg = true);
    static void UpdateOTAInternetURL(void *pvParameters);
    void UpdateOTAFileUpload();

    void AddToUpdateString(String update_msg);
    void AddToErrorString(String error_msg);

    bool _str_msg = true;

    String _server_json_url;
    String _update_url;
    int server_port;
    bool _update_success = false;

    bool _auto_restart = false;
    String _error_msg;
    String _update_msg;

    bool _update_running = false;
    JsonDocument _update_meta_json;
    A76XX *_a76xx = nullptr;
    A76XXHTTPClient *_a76xx_http = nullptr;
    HTTPClient *_wifi_http;
    
    ConfigHelper *_config_helper;
    
    
};

#endif // OTA_HELPER_H