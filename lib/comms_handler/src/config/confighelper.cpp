#include "config/confighelper.h"
#include <esp_log.h>
#include <WiFi.h>

static const char *TAG = "ConfigHelper";

/**
 * @brief Construct a new Config Helper:: Config Helper object
 * 
 * @param factoryreset overwrite the current config (if exists) with the default config
 */
ConfigHelper::ConfigHelper(bool factoryreset)
{
    if (factoryreset)
    {
        restoreDefaultConfigJSON(true);
    }
    else
    {
        // First have to create the default config to know what to load
        restoreDefaultConfigJSON();
        loadConfigJSONFromNVS();
    }
}

ConfigHelper::~ConfigHelper()
{
    _config_json.clear();
}

/**
 * @brief Helper function to get device MAC as a string
 * 
 * String has special characters already removed
 * 
 * @return String 
 */
String ConfigHelper::getDeviceMACString(){
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    ESP_LOGI(TAG, "Retrieved MAC Address as string [%s]", mac.c_str());
    return mac;
}

/**
 * @brief All the default options of the configuration set to the JSON object
 * 
 * JSON object is cleared first
 * Formats strings with the Device MAC string where necessary
 * 
 * @param write_to_nvs 
 */
void ConfigHelper::restoreDefaultConfigJSON(bool write_to_nvs)
{
    // Is the clear necessary?
    _config_json.clear();

    /* ---------------------- DEFAULT CONFIGURATION VALUES ---------------------- */
    _config_json[DEVICE_UID_KEY] = getDeviceMACString();
    ESP_LOGI(TAG, "Factory UID (MAC Address) [%s]",  _config_json[DEVICE_UID_KEY].as<String>().c_str());

    _config_json[DEVICE_NAME_KEY] = "SLT DL Debug NB IoT";

    // Factory SSID is NB_IoT_<MAC>
    String apssid_temp = "NB_IoT_" + _config_json[DEVICE_UID_KEY].as<String>();
    _config_json[ACCESSPOINT_SSID_KEY] = apssid_temp;
    _config_json[ACCESSPOINT_PASS_KEY] = "";

    // Internet Access
    _config_json[NET_PRIORITY_PREF_KEY] = NET_WIFI_THEN_LTE_PREF_OPTION;
    _config_json[NET_PRIORITY_SWITCH_TIME_KEY] = 2;

    _config_json[STATION_SSID_KEY] = "Hamza Pixel 6a";
    _config_json[STATION_PASS_KEY] = "ham54321";

    _config_json[A76XX_APN_NAME_KEY] = "internet";


    // MQTT Configurations
    _config_json[MQTT_CLIENT_ID_KEY] = _config_json[DEVICE_UID_KEY];
    _config_json[MQTT_USERNAME_KEY] = "";
    _config_json[MQTT_PASSWORD_KEY] = "";
    _config_json[MQTT_SERVER_URL_KEY] = "test.mosquitto.org";
    _config_json[MQTT_SERVER_PORT_KEY] = 1883;

    _config_json[MQTT_KEEPALIVE_KEY] = 60;
    _config_json[MQTT_CLEAN_SESSION_KEY] = true;
    _config_json[MQTT_RETAIN_KEY] = false;

    _config_json[MQTT_LWT_TOPIC_KEY] = _config_json[DEVICE_UID_KEY].as<String>() + "/lwt";
    _config_json[MQTT_LWT_PAYLOAD_KEY] = "offline";
    _config_json[MQTT_LWT_QOS_KEY] = 0;
    _config_json[MQTT_QUEUE_SIZE_KEY] = 30;

    _config_json[CONFIG_PORTAL_USERNAME_KEY] = "admin";
    _config_json[CONFIG_PORTAL_PASSWORD_KEY] = "admin";


    if (write_to_nvs)
    {
        saveConfigJSONToNVS();
        ESP_LOGI(TAG, "Restored default config and saved to NVS");
    }else{
        ESP_LOGI(TAG, "Restored default config");
    }
}

/**
 * @brief Load the configuration from NVS to JSON object
 * 
 */
void ConfigHelper::loadConfigJSONFromNVS()
{
    ESP_LOGI(TAG, "Loading config JSON from NVS");
    _preferences.begin(_config_namespace, true);
    for (JsonPair kv : _config_json.as<JsonObject>())
    {
        const char *key = kv.key().c_str();
        if (_preferences.isKey(key))
        {
            getConfigOptionNVS(key, false);
        }
    }

    // TODO: handle if factory options have more keys than in NVS
    _preferences.end();
}

/**
 * @brief Save the configuration JSON to NVS
 * 
 */
void ConfigHelper::saveConfigJSONToNVS()
{
    ESP_LOGI(TAG, "Saving config JSON to NVS");
    _preferences.begin(_config_namespace, false);
    for (JsonPair kv : _config_json.as<JsonObject>())
    {
        const char *key = kv.key().c_str();
        setConfigOptionNVS(key, false);
    }
    _preferences.end();
}

/**
 * @brief Get the JSON string of the configuration
 * 
 * @return String 
 */
String ConfigHelper::getConfigJSONString()
{
    String jsonstring;
    // TODO: remove pretty, pretty is for debugging only
    serializeJsonPretty(_config_json, jsonstring);
    
    return jsonstring;
}

/* ------------------------ GET CONFIG VALUE FROM NVS ----------------------- */
/**
 * @brief Writes the value of the key from NVS to the value of the key in the JSON object
 * 
 * @param key the key to get from NVS
 * @param open_namespace flag to open the namespace, set to false if handled externally
 */
void ConfigHelper::getConfigOptionNVS(const char *key, bool open_namespace)
{
    // Only four data types for now
    
    if(open_namespace){
        // Avoid repeating starting of the namespace
        _preferences.begin(_config_namespace, true);
    }

    if (_config_json[key].is<int>())
    {
        _config_json[key] = _preferences.getInt(key, _config_json[key].as<int>());
        ESP_LOGI(TAG, "[%d] (int) : key [%s]", _config_json[key], key);
    }
    else if (_config_json[key].is<String>())
    {
        _config_json[key] = _preferences.getString(key, _config_json[key].as<String>());
        ESP_LOGI(TAG, "[%s] (String) : key [%s]", _config_json[key].as<String>().c_str(), key);
    }
    else if (_config_json[key].is<bool>())
    {
        _config_json[key] = _preferences.getBool(key, _config_json[key].as<bool>());
        ESP_LOGI(TAG, "[%s] (bool) : key [%s]", _config_json[key] ? "true" : "false", key);
    }
    else if (_config_json[key].is<float>())
    {
        _config_json[key] = _preferences.getFloat(key, _config_json[key].as<float>());
        ESP_LOGI(TAG, "[%f] (float) : key [%s]", _config_json[key], key);
    }
    else
    {
        ESP_LOGE(TAG, "Unsupported data type for key: %s", key);
    }

    if(open_namespace){
        _preferences.end();
    }
}

/* ------------------------- SET CONFIG VALUE IN NVS ------------------------ */
/**
 * @brief Writes the value of the key from the JSON object to the NVS
 * 
 * @param key the key to write to NVS (15 char limit?)
 * @param open_namespace flag to open the namespace, set to false if handled externally
 */
void ConfigHelper::setConfigOptionNVS(const char *key, bool open_namespace)
{
    if(open_namespace){
        // Avoid repeating starting of the namespace
        _preferences.begin(_config_namespace, false);
    }
    size_t retval;

    // Only four data types for now
    if (_config_json[key].is<int>())
    {
        retval = _preferences.putInt(key, _config_json[key].as<int>());
        ESP_LOGI(TAG, "[%d] (int) : key [%s] retval: [%d]", _config_json[key], key, retval);
    }
    else if (_config_json[key].is<String>())
    {
        retval = _preferences.putString(key, _config_json[key].as<String>());
        ESP_LOGI(TAG, "[%s] (String) : key [%s] retval: [%d]", _config_json[key].as<String>().c_str(), key, retval);
    }
    else if (_config_json[key].is<bool>())
    {
        retval = _preferences.putBool(key, _config_json[key].as<bool>());
        ESP_LOGI(TAG, "[%s] (bool) : key [%s] retval: [%d]", _config_json[key] ? "true" : "false", key, retval);
    }
    else if (_config_json[key].is<float>())
    {
        retval = _preferences.putFloat(key, _config_json[key].as<float>());
        ESP_LOGI(TAG, "[%f] (float) : key [%s] retval: [%d]", _config_json[key], key, retval);
    }
    else
    {
        ESP_LOGE(TAG, "Unsupported data type for key: %s", key);
    }

    if(open_namespace){
        _preferences.end();
    }
}

/* ----------------- PUBLICLY ACCESS CONFIG OPTION FUNCTIONS ---------------- */

/* ---------------------------- GETTER FUNCTIONS ---------------------------- */
// Get an integer value from the configuration
int ConfigHelper::getConfigOption(const char *key, int default_value)
{
    // No need to access Preferences for this
    if (_config_json[key].is<int>())
    {
        return _config_json[key].as<int>();
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not an int", key);
        return default_value;
    }
}

// Get a string value from the configuration
String ConfigHelper::getConfigOption(const char *key, const char *default_value)
{
    // No need to access Preferences for this
    if (_config_json[key].is<String>())
    {
        return _config_json[key].as<String>();
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not a String", key);
        return default_value;
    }
}

// Get a boolean value from the configuration
bool ConfigHelper::getConfigOption(const char *key, bool default_value)
{
    // No need to access Preferences for this
    if (_config_json[key].is<bool>())
    {
        return _config_json[key].as<bool>();
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not a bool", key);
        return default_value;
    }
}

// Get a float value from the configuration
float ConfigHelper::getConfigOption(const char *key, float default_value)
{
    // No need to access Preferences for this
    if (_config_json[key].is<float>())
    {
        return _config_json[key].as<float>();
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not a float", key);
        return default_value;
    }
}

/* ---------------------------- SETTER FUNCTIONS ---------------------------- */
// Set an integer value in the configuration
void ConfigHelper::setConfigOption(const char *key, int value, bool write_to_nvs)
{
    if (_config_json[key].is<int>())
    {
        _config_json[key] = value;
        if (write_to_nvs)
            setConfigOptionNVS(key);
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not an int", key);
    }
}

// Set a string value in the configuration
void ConfigHelper::setConfigOption(const char *key, const char *value, bool write_to_nvs)
{
    if (_config_json[key].is<String>())
    {
        String temp = value;
        _config_json[key] = temp;
        ESP_LOGI(TAG, "Set [%s] to [%s]", key, _config_json[key].as<String>().c_str());
        if (write_to_nvs)
            setConfigOptionNVS(key);
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not a String", key);
    }
}

// Set a boolean value in the configuration
void ConfigHelper::setConfigOption(const char *key, bool value, bool write_to_nvs)
{
    if (_config_json[key].is<bool>())
    {
        _config_json[key] = value;
        if (write_to_nvs)
            setConfigOptionNVS(key);
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not a bool", key);
    }
}

// Set a float value in the configuration
void ConfigHelper::setConfigOption(const char *key, float value, bool write_to_nvs)
{
    if (_config_json[key].is<float>())
    {
        _config_json[key] = value;
        if (write_to_nvs)
            setConfigOptionNVS(key);
    }
    else
    {
        ESP_LOGE(TAG, "Value of Key: [%s] is not a float", key);
    }
}