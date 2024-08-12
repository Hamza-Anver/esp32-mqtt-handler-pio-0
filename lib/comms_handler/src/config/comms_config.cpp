#include "comms_config.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_log.h>

void setDefaultJsonConfig(JsonDocument *config){
    (*config)["apssid"] = "ESP_32_AP_mac";
    (*config)["appass"] = "";
    (*config)["stassid"] = "Hamza Pixel 6a";
    (*config)["stapass"] = "ham54321";
};