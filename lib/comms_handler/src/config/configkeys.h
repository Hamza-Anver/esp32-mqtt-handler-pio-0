#ifndef CONFIG_KEYS_H
#define CONFIG_KEYS_H

// KEY STRINGS FOR CONFIGURATIONS
// DEFAULT VALUES FOR THE CONFIGURATIONS ARE IN THE CPP FILE
#define DEVICE_UID_KEY "deviceid"
#define DEVICE_NAME_KEY "name"
#define CONFIG_PORTAL_USERNAME_KEY "pusername"
#define CONFIG_PORTAL_PASSWORD_KEY "ppassword"

#define ACCESSPOINT_SSID_KEY "apssid"
#define ACCESSPOINT_PASS_KEY "appass"

#define UID_REPLACEMENT_PATTERN "{UID}"

#define NET_PRIORITY_PREF_KEY "netpref"
#define NET_PRIORITY_SWITCH_TIME_KEY "netinterval"

#define NET_WIFI_ONLY_PREF_OPTION "wifionly"
#define NET_LTE_ONLY_PREF_OPTION "lteonly"
#define NET_WIFI_THEN_LTE_PREF_OPTION "ltethenwifi"
#define NET_LTE_THEN_WIFI_PREF_OPTION "wifithenlte"

#define STATION_SSID_KEY "stassid"
#define STATION_PASS_KEY "stapass"

#define A76XX_APN_NAME_KEY "apnname"

#define MQTT_CLIENT_ID_KEY "mqttid"
#define MQTT_USERNAME_KEY "mqttuser"
#define MQTT_PASSWORD_KEY "mqttpass"
#define MQTT_SERVER_URL_KEY "mqtturl"
#define MQTT_SERVER_PORT_KEY "mqttport"

#define MQTT_KEEPALIVE_KEY "mkeepalive"
#define MQTT_CLEAN_SESSION_KEY "mclean"
#define MQTT_RETAIN_KEY "mretain"

#define MQTT_CLEAN_SESSION_TRUE_OPTION "cleansessiontrue"
#define MQTT_CLEAN_SESSION_FALSE_OPTION "cleansessionfalse"

#define MQTT_RETAIN_TRUE_OPTION "retaintrue"
#define MQTT_RETAIN_FALSE_OPTION "retainfalse"

#define MQTT_LWT_TOPIC_KEY "lwttopic"
#define MQTT_LWT_PAYLOAD_KEY "lwtpayload"
#define MQTT_LWT_QOS_KEY "lwtqos"
#define MQTT_QUEUE_SIZE_KEY "mqttqsize"


#define OTA_JSON_URL_KEY "otajsonurl"
#define OTA_BIN_URL_KEY "otabinurl"
#define OTA_UPDATE_FREQUENCY_KEY "otafreq"



#endif // FR_CONFIG_H