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
    void MQTT_queue_pub(const char *topic, const char *message, int qos);
    bool MQTT_sub(const char *topic);
    static void MQTT_manage_task(void *pvParameters);
    void MQTT_init();

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