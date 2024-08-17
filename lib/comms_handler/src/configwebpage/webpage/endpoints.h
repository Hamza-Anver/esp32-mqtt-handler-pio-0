#ifndef ENDPOINTS_H
#define ENDPOINTS_H


#define INTERNET_PREF_CONFIG_ENDPOINT "/netpref"

#define STA_START_SCAN_ENDPOINT "/startscan"
#define STA_SCAN_RESULTS_ENDPOINT "/getscan"
#define STA_SET_CONFIG_ENDPOINT "/stacfg"
#define STA_SEND_UPDATE_ENDPOINT "/stacfgupdate"

#define LTE_SET_CONFIG_ENDPOINT "/ltecfg"

#define MQTT_SET_CONFIG_ENDPOINT "/mqttcfg"

#define AP_SET_CONFIG_ENDPOINT "/apsetconfig"

#define FACTORY_RESET_ENDPOINT "/factoryreset"
#define RESTART_DEVICE_ENDPOINT "/restartdevice"

#define UPDATE_FILE_UPLOAD_ENDPOINT "/updateota"
#define UPDATE_OTA_BIN_URL_ENDPOINT "/binurlotanow"

#define UPDATE_OTA_CONFIG_ENDPOINT "/updateotaconfig"

#define UPDATE_OTA_STATUS_ENDPOINT "/getotastatus"

#define CONFIG_JSON_GET_ENDPOINT "/cfggetjson"
#define CONFIG_JSON_SET_ENDPOINT "/configsetjson"

#define DEVICE_GET_STATUS_ENDPOINT "/getstatus"
#define DEVICE_SET_SETTINGS_ENDPOINT "/devicecfg"


#endif // ENDPOINTS_H