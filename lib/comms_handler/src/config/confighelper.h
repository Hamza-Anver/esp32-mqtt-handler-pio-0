#ifndef CONFIGHELPER_H
#define CONFIGHELPER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config/configkeys.h"


class ConfigHelper {
    public:
        ConfigHelper(bool factoryreset);
        ~ConfigHelper();

        String getDeviceMACString();

        void restoreDefaultConfigJSON(bool write_to_nvs = false);
        void loadConfigJSONFromNVS();
        void saveConfigJSONToNVS();

        String getConfigJSONString();

        //TODO: handle a string of JSON (bulk assignment)

        // GETTERS
        // Value is needed to help with type
        int getConfigOption(const char *key, int default_value);
        String getConfigOption(const char *key, const char *default_value);
        bool getConfigOption(const char *key, bool default_value);
        float getConfigOption(const char *key, float default_value);

        // SETTERS
        void setConfigOption(const char *key, int value, bool write_to_nvs = true);
        void setConfigOption(const char *key, const char *value, bool write_to_nvs = true);
        void setConfigOption(const char *key, bool value, bool write_to_nvs = true);
        void setConfigOption(const char *key, float value, bool write_to_nvs = true);

    private:
        void getConfigOptionNVS(const char *key, bool open_namespace = true);
        void setConfigOptionNVS(const char *key, bool open_namespace = true);
        JsonDocument _config_json;
        const char *_config_namespace = "cfg";
        Preferences _preferences;
};

#endif // CONFIGHELPER_H