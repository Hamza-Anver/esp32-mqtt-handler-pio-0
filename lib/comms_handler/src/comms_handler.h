#ifndef _COMMS_HANDLER
#define _COMMS_HANDLER

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <MQTT.h>
#include <A76XX.h>

// Internal components
#include <metaheaders.h>
#include CONFIG_KEYS_HEADER
#include VERSION_HEADER

#include <ota/otahelper.h>
#include <config/confighelper.h>
#include <configwebpage/configwebpage.h>

/* -------------------------------- DEFINITIONS -------------------------------- */

#define A76XX_PIN_TX 17
#define A76XX_PIN_RX 16
#define A76XX_PWR_PIN 12
#define A76XX_RST_PIN 4

#define SERIAL_LTE Serial1
#define LTE_TIMEOUT 10000
#define LTE_COMP_TIMEOUT 5000
#define LTE_PUBLISH_TIMEOUT_S 30

#define COMMS_INDICATOR_LED LED_BUILTIN

/* -------------------------------- CLASS DEFINITION -------------------------------- */

class CommsHandler
{
public:
    // Data types
    // Internet preferences
    enum CommsHandler_Net_Pref_t
    {
        WiFi_Only,
        LTE_Only,
        WiFi_then_LTE,
        LTE_then_WiFi
    };

    // All states of operation
    enum CommsHandler_State_t
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
    enum CommsHandler_Error_t
    {
        Comms_No_Error,
        Comms_Error_WiFi,
        Comms_Error_LTE,
        Comms_Error_MQTT,
        Comms_Error_Unknown
    };

    struct CommsHandler_MQTTMessage_t
    {
        String topic;
        String payload;
        int qos;
        bool retain;
        bool dup;
    };

    // TODO: handle using existing clients (i.e. accept clients as arguments)
    //  ONLY CALL AFTER ARDUINO SETUP
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

    static void CommsHandlerManagementTask(void *pvParameters);
    static void CommsHandlerQueueTask(void *pvParameters);

    struct BlinkPattern_t
    {
        int num_blinks;
        int times[10]; // Max 10 blinks
    };

    void setIndicatorLEDPattern(int count,
                                ...);

    static void IndicatorLEDPatternTask(void *pvParameters);

private:
    void _load_current_config();

    void _a76xx_power_on();
    void _a76xx_mqtt_init();
    void _a76xx_mqtt_pub();
    void _a76xx_mqtt_sub();
    void _a76xx_power_off();

    void _wifi_init();

    // Sub components
    OTAHelper *_ota_helper;
    ConfigHelper *_config_helper;
    ConfigWebpage *_config_webpage;

    CommsHandler_State_t _state;
    CommsHandler_Error_t _error;
    CommsHandler_Net_Pref_t _net_preference;

    // Local variables
    QueueHandle_t _mqtt_msg_queue;
    QueueHandle_t _mqtt_msg_current;
    QueueHandle_t _led_blink_pattern;

    // Config variables
    String _mqtt_server;
    int _mqtt_port;
    String _mqtt_username;
    String _mqtt_password;
    String _mqtt_client_id;

    int _size_mqtt_msg_queue;

    // Clients
    A76XX *_a76xx;

    MQTTClient *_mqtt_client;
    WiFiClient *_wifi_client;
};

#endif // _COMMS_HANDLER