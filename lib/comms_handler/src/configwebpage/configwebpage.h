#ifndef CONFIG_WEBPAGE_H
#define CONFIG_WEBPAGE_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "config/confighelper.h"

#include <freertos/FreeRTOS.h>

#include <metaheaders.h>

#include CONFIG_WEBPAGE_GZIP_HEADER
#include CONFIG_KEYS_HEADER
#include ENDPOINTS_HEADER

class ConfigWebpage {
public:
    ConfigWebpage(ConfigHelper *config_helper = nullptr);
    
    static void ConfigServerTask(void *pvParameters);

    void ConfigServeRootPage(AsyncWebServerRequest *request);

    /* --------------------------- CALL BACK FUNCTIONS -------------------------- */

    void handleSendCurrentConfigJSON(AsyncWebServerRequest *request);
    void handleReceiveConfigJSON(AsyncWebServerRequest *request);
    void handleFactoryReset(AsyncWebServerRequest *request);
    void handleUpdateFirmwareOTA(AsyncWebServerRequest *request);

    void handleStationStartScan(AsyncWebServerRequest *request);
    void handleStationScanResults(AsyncWebServerRequest *request);
    void handleStationSetConfig(AsyncWebServerRequest *request);
    void handleStationSendUpdate(AsyncWebServerRequest *request);

    void handleAccessPointSetConfig(AsyncWebServerRequest *request);

    /* ---------------------- END OF CALL BACK FUNCTIONS ------------------------ */

    ConfigHelper *_config_helper;
    AsyncWebServer *_server;
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