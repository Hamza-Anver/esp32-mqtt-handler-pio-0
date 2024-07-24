#ifndef _COMMS_HANDLER
#define _COMMS_HANDLER

#include "comms_config.h"
#include <Arduino.h>
#include <A76XX.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include <Arduino.h>
#include <WiFi.h>
#include <MQTT.h>

enum WiFi_State
{
    WiFi_Fail,
    WiFi_Off,
    WiFi_Connected
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
    LTE_Connected

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
    static void MQTT_manage_task(void* pvParameters);
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

    Comm_State Comm_state;
    A76XX *LTE_modem;
    A76XXMQTTClient *LTE_mqtt;
    LTE_Module_State LTE_state;
    WiFi_State WiFi_state;
    WiFiClient *wifi_client;
    MQTTClient *mqtt_client;


    QueueHandle_t MQTT_pub_queue;

    unsigned int priority_reconnect_timeout;
};

#endif // _COMMS_HANDLER