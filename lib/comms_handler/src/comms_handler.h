#ifndef _COMMS_HANDLER
#define _COMMS_HANDLER

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <MQTT.h>
#include <A76XX.h>

/* -------------------------------- DEFINITIONS -------------------------------- */

#define A76XX_PIN_TX 17
#define A76XX_PIN_RX 16
#define A76XX_PWR_PIN 12
#define A76XX_RST_PIN 4

#define SERIAL_LTE Serial1
#define LTE_TIMEOUT 10000
#define LTE_COMP_TIMEOUT 5000
#define LTE_PUBLISH_TIMEOUT_S 30

// Internet preferences
enum InternetPreference_t
{
    WiFi_Only,
    LTE_Only,
    WiFi_then_LTE,
    LTE_then_WiFi
};

// All states of operation
enum CommsHandlerState_t
{
    Comms_Unititialized,
    Comms_WiFi_Connected_Only,
    Comms_LTE_Connected_Only,
    Comms_WiFi_Connected_Primary,
    Comms_LTE_Connected_Primary,
    Comms_WiFi_Connected_Secondary,
    Comms_LTE_Connected_Secondary,
    Comms_Disconnected
};

// All error states
enum CommsHandlerError_t
{
    Comms_No_Error,
    Comms_Error_WiFi,
    Comms_Error_LTE,
    Comms_Error_MQTT,
    Comms_Error_Unknown
};


/* -------------------------------- CLASS DEFINITION -------------------------------- */

class CommsHandler
{
public:
    // ONLY CALL AFTER ARDUINO SETUP
    CommsHandler();

    bool addMQTTMessageToQueue(String topic,
                               String payload,
                               int qos = 0);

    bool addMQTTMessageToQueue(String topic,
                               String payload,
                               int qos = 0,
                               bool retain = false,
                               bool dup = false);

    bool subscribeToMQTTTopic(String topic, int qos = 0);
    bool unsubscribeFromMQTTTopic(String topic);

    bool messageAvailableMQTTTopic(String topic);


    static void CommsHandlerQueueTask(void *pvParameters);

private:
    // Private functions
    void _a76xx_init();
    void _a76xx_mqtt_init();
    void _a76xx_mqtt_pub();

    void _wifi_init();


    // State variables
    String _mqtt_server;
    int _mqtt_port;
    String _mqtt_username;
    String _mqtt_password;
    String _mqtt_client_id;

    // Local variables
    QueueHandle_t _mqtt_message_queue;
    InternetPreference_t _net_preference;

    // Clients
    A76XX *_lte_modem;

    MQTTClient *_mqtt_client;
    WiFiClient *_wifi_client;

};

#endif // _COMMS_HANDLER