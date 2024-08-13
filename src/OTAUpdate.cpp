#include "OTAUpdate.h"

const char* OTAUpdate::TAG = "OTAUpdate";

OTAUpdate::OTAUpdate(const char* updateUrl) {
    this->updateUrl = updateUrl;
}

void OTAUpdate::performUpdate() {
    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(TAG, "No WiFi connection. OTA update aborted.");
        return;
    }

    ESP_LOGI(TAG, "Starting OTA update...");
    
    HTTPClient httpClient;
    httpClient.begin(updateUrl);
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
                    ESP_LOGI(TAG, "Update successfully completed. Rebooting...");
                    ESP.restart();
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
}

