#define DEBUG_SERIAL true
#define DEBUG_TIMEOUT 5000
#if defined DEBUG_SERIAL
   #define debug_begin(x)        Serial.begin(x)
   #define debug(x)                   Serial.print(x)
   #define debugln(x)                 Serial.println(x)
#else
   #define debug_begin(x)
   #define debug(x)
   #define debugln(x)
#endif

// Device configuration
#define UUID 100

#define LTE_PIN_TX 17
#define LTE_PIN_RX 16
#define LTE_PWR_PIN 12
#define LTE_RST_PIN 4

// MQTT configuration
#define MQTT_CLIENT_ID "default_client_test"
#define QOS_LEVEL 0
#define MQTT_PORT 1883
#define MQTT_KEEPALIVE 15
#define MQTT_CLEAN_SESSION true
#define MQTT_USE_SSL false
#define MQTT_RETAIN false

#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_LWT_TOPIC "dying-nodes"
#define MQTT_LWT_MESSAGE "ded"
#define MQTT_LWT_QOS 2

// LTE configuration
#define LTE_APN "mobitel"
#define SERIAL_LTE Serial1
#define LTE_TIMEOUT 10000
#define LTE_COMP_TIMEOUT 5000
#define LTE_PUBLISH_TIMEOUT_S 30

// default AP settings
#define WIFI_SSID_AP "config_1337_100"
#define WIFI_PASS_AP "password"

// Debug STA settings
#define WIFI_SSID_DEF "Hamza Pixel 6a"
#define WIFI_PASS_DEF "ham54321"


// Prioritize using WiFi if possible, show AP and creds page if no WiFi
// Attempt reconnecting to WiFi every x seconds
#define WIFI_USE_AP true
#define WIFI_PRIORITIZED true

// Interval in ms to check again to switch back to primary comms method
#define PRIMARY_CHECK_TIME_MS 600000

