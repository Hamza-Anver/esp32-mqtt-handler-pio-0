#include "config/confighelper.h"
#include <esp_log.h>
#include <WiFi.h>

static const char *TAG = "ConfigHelper";

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

String ConfigHelper::getDeviceMACString(){
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    ESP_LOGI(TAG, "Retrieved MAC Address as string [%s]", mac.c_str());
    return mac;
}

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
    _config_json[STATION_SSID_KEY] = "Hamza Pixel 6a";
    _config_json[STATION_PASS_KEY] = "ham54321";

    if (write_to_nvs)
    {
        saveConfigJSONToNVS();
        ESP_LOGI(TAG, "Restored default config and saved to NVS");
    }else{
        ESP_LOGI(TAG, "Restored default config");
    }
}

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

String ConfigHelper::getConfigJSONString()
{
    String jsonstring;
    // TODO: remove pretty, pretty is for debugging only
    serializeJsonPretty(_config_json, jsonstring);
    
    return jsonstring;
}

/* ------------------------ GET CONFIG VALUE FROM NVS ----------------------- */
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