#ifndef _COMMS_HANDLER
#define _COMMS_HANDLER

#include "config/comms_config.h"
#include <Arduino.h>
#include <A76XX.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>
#include <WiFi.h>
#include <MQTT.h>

#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <Update.h>

enum WiFi_State
{
    WiFi_Fail,
    WiFi_Off,
    WiFi_Connected,
    WiFi_Disabled
};

enum Comm_State
{
    Comm_Fail,
    Comm_Off,
    Comm_On,
    Comm_LTE,
    Comm_WiFi
};

enum LTE_Module_State
{
    LTE_Fail,
    LTE_Off,
    LTE_Connected,
    LTE_Disabled

};

struct MQTT_pub_t
{
    char *topic;
    char *message;
    int qos;
};

class CommsHandler
{
public:
    void createJsonConfig(bool factoryreset);
    void writeJsonToNVS(JsonDocument *jsonsrc = nullptr);
    String getConfigOptionString(const char *option, const char *default_value);
    bool setConfigOptionString(const char *option, const char *value);

    // Configuration callbacks
    void sendConfigJsonCallback(AsyncWebServerRequest *request);
    void handleConfigJsonCallback(AsyncWebServerRequest *request);
    void factoryResetConfigCallback(AsyncWebServerRequest *request);

    void WiFi_config_page_init();
    static void WiFi_config_page_task(void *pvParameters);
    void WiFi_config_handle_root(AsyncWebServerRequest *request);

    // Page callbacks
    void StationScanCallbackStart(AsyncWebServerRequest *request);
    void StationScanCallbackReturn(AsyncWebServerRequest *request);
    void StationSetConfig(AsyncWebServerRequest *request);
    void StationSendUpdate(AsyncWebServerRequest *request);

    void AccessPointSetConfig(AsyncWebServerRequest *request);


    // Temp test function
    static void TestOTAInternet(void *pvParameters);
    void TestOTAInternetOutput(String message);

    void MQTT_queue_pub(const char *topic, const char *message, int qos);
    bool MQTT_sub(const char *topic);
    static void MQTT_manage_task(void *pvParameters);
    void MQTT_init();

    // Server stuff
    AsyncWebServer *_server;
    DNSServer *_dnsServer;
    IPAddress _ap_ip;
    IPAddress _net_msk;
    const byte _dns_port = 53;

private:
    bool LTE_init();
    bool LTE_connect();
    bool LTE_pub(const char *topic, const char *message, int qos);
    bool LTE_disable();

    bool WiFi_init();
    bool WiFi_connect();
    bool WiFi_connected();
    bool WiFi_pub(const char *topic, const char *message, int qos);
    bool WiFi_disable();

    Preferences _configops;
    const char *_conf_namespace = "comms_handler";
    JsonDocument _config_json;

    int wifi_config_attempts;

    Comm_State Comm_state;

    A76XX *LTE_modem;
    LTE_Module_State LTE_state;

    WiFi_State WiFi_state;
    WiFiClient *WiFi_client;

    A76XXMQTTClient *LTE_mqtt;
    MQTTClient *WiFi_mqtt;

    QueueHandle_t MQTT_pub_queue;

    unsigned int priority_reconnect_timeout;
    bool provisioner_running;
};

#endif // _COMMS_HANDLER