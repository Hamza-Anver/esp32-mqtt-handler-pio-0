#ifndef CONFIG_KEYS_H
#define CONFIG_KEYS_H

// KEY STRINGS FOR CONFIGURATIONS
// DEFAULT VALUES FOR THE CONFIGURATIONS ARE IN THE CPP FILE
#define DEVICE_UID_KEY "deviceid"
#define DEVICE_NAME_KEY "name"

#define ACCESSPOINT_SSID_KEY "apssid"
#define ACCESSPOINT_PASS_KEY "appass"

#define UID_REPLACEMENT_PATTERN "{UID}"

#define NET_PRIORITY_PREF_KEY "prefconnect"

#define NET_WIFI_ONLY_PREF_OPTION "wifionly"
#define NET_LTE_ONLY_PREF_OPTION "lteonly"
#define NET_WIFI_THEN_LTE_PREF_OPTION "ltethenwifi"
#define NET_LTE_THEN_WIFI_PREF_OPTION "wifithenlte"

#define STATION_SSID_KEY "stassid"
#define STATION_PASS_KEY "stapass"

#define OTA_URL_KEY "otaurl"
#define OTA_UPDATE_FREQUENCY_KEY "otafreq"

// IDS for references in the HTML file
#define WIFI_STATUS_MSG_ID "wifistatus"
#define LTE_STATUS_MSG_ID "ltestatus"
#define DEVICE_UID_MSG_ID "deviceuid"
#define DEVICE_UPTIME_MSG_ID "deviceuptime"

//TODO: add more here later



#endif // FR_CONFIG_H