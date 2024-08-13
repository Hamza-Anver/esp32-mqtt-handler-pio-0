#ifndef FR_CONFIG_H
#define FR_CONFIG_H

// KEY STRINGS FOR CONFIGURATIONS
// DEFAULT VALUES FOR THE CONFIGURATIONS ARE IN THE CPP FILE
#define ACCESSPOINT_SSID_KEY "apssid"
#define ACCESSPOINT_PASS_KEY "appass"
#define STATION_SSID_KEY "stassid"
#define STATION_PASS_KEY "stapass"


// ENDPOINT DECLARATIONS
#define STA_START_SCAN_ENDPOINT "/startscan"
#define STA_SCAN_RESULTS_ENDPOINT "/getscan"
#define STA_SET_CONFIG_ENDPOINT "/stationcfg"
#define STA_SEND_UPDATE_ENDPOINT "/stationcfgupdate"

#define AP_SET_CONFIG_ENDPOINT "/apsetconfig"

#define FACTORY_RESET_ENDPOINT "/factoryreset"

#define UPDATE_FIRMWARE_ENDPOINT "/updateota"

#define CONFIG_JSON_GET_ENDPOINT "/cfggetjson"
#define CONFIG_JSON_SET_ENDPOINT "/configsetjson"


#endif // FR_CONFIG_H