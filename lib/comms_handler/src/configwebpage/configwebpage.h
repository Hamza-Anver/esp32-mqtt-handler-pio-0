#ifndef CONFIG_WEBPAGE_H
#define CONFIG_WEBPAGE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "config/confighelper.h"
#include "ota/otahelper.h"

#include <freertos/FreeRTOS.h>

class ConfigWebpage {
public:
    ConfigWebpage(ConfigHelper *config_helper = nullptr, OTAHelper *ota_helper = nullptr);
    
    static void ConfigServerTask(void *pvParameters);

    void ConfigServeRootPage(AsyncWebServerRequest *request);

    /* --------------------------- CALL BACK FUNCTIONS -------------------------- */

    void handleSendCurrentConfigJSON(AsyncWebServerRequest *request);
    void handleReceiveConfigJSON(AsyncWebServerRequest *request);
    void handleSendStatusJSON(AsyncWebServerRequest *request);

    void handleDeviceSettingsConfig(AsyncWebServerRequest *request);
    void handleFactoryReset(AsyncWebServerRequest *request);
    void handleDeviceRestart(AsyncWebServerRequest *request);

    void handleUpdateOTAConfig(AsyncWebServerRequest *request);
    static void handleUpdateOTAStatus(void *pvParameters);
    void handleUpdateOTAInternet(AsyncWebServerRequest *request);
    void handleUpdateOTAFileUpload(AsyncWebServerRequest *reques, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleUpdateOTANowRequest(AsyncWebServerRequest *request);

    void handleReceiveInternetPreferences(AsyncWebServerRequest *request);

    void handleStationStartScan(AsyncWebServerRequest *request);
    void handleStationScanResults(AsyncWebServerRequest *request);
    void handleStationSetConfig(AsyncWebServerRequest *request);
    static void handleStationSendUpdate(void *pvParameters);

    void handleLTESetConfig(AsyncWebServerRequest *request);

    void handleAccessPointSetConfig(AsyncWebServerRequest *request);

    void handleMQTTSetConfig(AsyncWebServerRequest *request);

    /* ---------------------- END OF CALL BACK FUNCTIONS ------------------------ */

    ConfigHelper *_config_helper;
    JsonDocument _update_json;
    OTAHelper *_ota_helper;
    AsyncWebServer *_server;
    AsyncEventSource *_event_source;
    DNSServer *_dnsServer;
    IPAddress _ap_ip;
    IPAddress _net_msk;
    const byte _dns_port = 53;

private:

    int _sta_attempts;
    String _sta_ssid;
    String _sta_pass;
};

#endif // CONFIG_WEBPAGE_H