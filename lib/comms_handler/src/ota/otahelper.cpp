#include "otahelper.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include <Update.h>

static const char *TAG = "OTAHelper";

OTAHelper::OTAHelper(ConfigHelper *config_helper) {
    if(config_helper == nullptr){
        _config_helper = new ConfigHelper(false);
    } else {
        _config_helper = config_helper;
    }
}


void OTAHelper::CallOTAInternetUpdateAsync(String updateURL){
    _update_url = updateURL;
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(TAG, "No WiFi connection. OTA update aborted.");
        vTaskDelete(NULL);
    }else{
        ESP_LOGI(TAG, "Starting asynchronous OTA update");
        xTaskCreate(UpdateOTAInternetURL, "UpdateOTAInternetURL", 8192, this, 1, NULL);
    }
    
}

void OTAHelper::UpdateOTAInternetURL(void* pvParameters) {
    OTAHelper *instance = (OTAHelper *)pvParameters;
    String updateURL = instance->_update_url;
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(TAG, "No WiFi connection. OTA update aborted.");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Starting OTA update...");
    
    HTTPClient httpClient;
    httpClient.begin(updateURL);
    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_OK) {
        int contentLength = httpClient.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            WiFiClient * client = httpClient.getStreamPtr();
            size_t written = Update.writeStream(*client);

            if (written == contentLength) {
                ESP_LOGI(TAG, "Written: %u successfully", written);
            } else {
                ESP_LOGE(TAG, "Written only: %u/%u. Update failed.", written, contentLength);
                return;
            }

            if (Update.end()) {
                if (Update.isFinished()) {
                    ESP_LOGI(TAG, "Update successfully completed. Reboot to apply changes.");
                } else {
                    ESP_LOGE(TAG, "Update not finished. Something went wrong!");
                }
            } else {
                ESP_LOGE(TAG, "Update failed. Error #: %d", Update.getError());
            }
        } else {
            ESP_LOGE(TAG, "Not enough space to begin OTA update");
        }
    } else {
        ESP_LOGE(TAG, "Firmware download failed, HTTP response: %d", httpCode);
    }

    httpClient.end();
    vTaskDelete(NULL);
}

