#include "otahelper.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include <Update.h>

static const char *TAG = "OTAHelper";

// TODO: Work with bin uploads
// TODO: Filesystem updates
// TODO: JSON checking

OTAHelper::OTAHelper(A76XX *a76xx)
{
    _a76xx = a76xx;
}

bool OTAHelper::CheckOTAHasMessage()
{
    return (_update_msg != "" ||
            _error_msg != "");
}

/**
 * @brief Writes the OTA message to the message string and clears the message
 *
 * @param message String to write the message to
 * @return true if the message is an update message (i.e. success)
 * @return false if the message is an error message
 */
bool OTAHelper::GetOTAMessage(String &message)
{
    if (_update_msg != "")
    {
        message = _update_msg;
        _update_msg = "";
        return true;
    }
    else if (_error_msg != "")
    {
        message = _error_msg;
        _error_msg = "";
        return false;
    }
    return false;
}

bool OTAHelper::CheckUpdateJSON(String jsonurl, OTAHelper::OTAMethod_t method)
{
    ESP_LOGI(TAG, "Checking JSON for OTA [%s]", jsonurl.c_str());
    if (method == OTA_WIFI)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            _error_msg += "No WiFi connection. Checking JSON aborted";
            ESP_LOGE(TAG, "No WiFi connection. Checking JSON aborted");
            return false;
        }
        _wifi_http = new HTTPClient();
        if (_wifi_http->begin(jsonurl.c_str()))
        {
            int httpCode = _wifi_http->GET();
            if (httpCode == HTTP_CODE_OK)
            {
                if (_wifi_http->getSize() > 6000)
                {
                    _error_msg += "File found at URL size too big for JSON!";
                    return false;
                };
                String payload = _wifi_http->getString();
                ESP_LOGI(TAG, "JSON received: %s", payload.c_str());
                if (deserializeJson(_update_meta_json, payload) == DeserializationError::Ok)
                {
                    serializeJsonPretty(_update_meta_json, payload);
                    _update_msg += "Received JSON: <br>" + payload;
                    return true;
                }
                else
                {
                    _error_msg = "JSON parsing from " + jsonurl + " failed!";
                    ESP_LOGE(TAG, "JSON parsing failed");
                    return false;
                }
            }
            else
            {
                _error_msg += "HTTP error: " + String(httpCode);
                ESP_LOGE(TAG, "HTTP error: %d", httpCode);
                _wifi_http->end();
                return false;
            }
            _wifi_http->end();
            return true;
        }else{
            _error_msg += "Failed to start the WiFi HTTP Client with URL: [" + jsonurl + "]";
            return false;
        }
    }
    else if (method == OTA_LTE)
    {
        _error_msg += "LTE OTA not implemented yet";
        ESP_LOGE(TAG, "LTE OTA not implemented yet");
        return false;
    }
    return false;
}

void OTAHelper::handleOTAChunks(String filename, size_t index, uint8_t *data, size_t len, bool final, size_t reqlen)
{
    if (!Update.isRunning())
    {
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            _error_msg = "Update could not begin. Error #: " + String(Update.getError());
            ESP_LOGE(TAG, "Update could not begin [%d]", Update.getError());
            return;
        }
        else
        {
            _update_msg = "Update started via chunks";
            ESP_LOGI(TAG, "Update started via chunks");
            _update_running = true;
        }
    }

    if (Update.write(data, len) != len)
    {
        _error_msg = "Update write failed";
        ESP_LOGE(TAG, "Update write failed");
        return;
    }
    else
    {
        _update_msg = "Writing file '" + filename + "' to device";
        _update_msg += "<br> Progress: " + String((index + len) * 100 / reqlen) + "% [" + String(index + len) + "/" + String(reqlen) + "] bytes";
    }

    if (final)
    {
        if (Update.end(true))
        {
            _update_msg = "OTA update completed. Reboot to apply changes.";
            ESP_LOGI(TAG, "OTA update completed. Reboot to apply changes.");
            _update_success = true;
        }
        else
        {
            ESP_LOGE(TAG, "OTA update failed. Error #: %d", Update.getError());
            _error_msg = "OTA update failed. Error #: " + String(Update.getError());
        }
    }
    return;
}

void OTAHelper::CallOTAInternetUpdateAsync(String updateURL)
{
    // TODO: Make this work with A76XX HTTP too
    // using a76xx http may be hard
    // Can use the MQTT stream to write chunks instead
    _update_url = updateURL;
    _error_msg = "";
    if (WiFi.status() != WL_CONNECTED)
    {
        ESP_LOGE(TAG, "No WiFi connection. OTA update aborted.");
        _error_msg = "No WiFi connection";
    }
    else
    {
        ESP_LOGI(TAG, "Starting asynchronous OTA update");
        xTaskCreate(UpdateOTAInternetURL, "UpdateOTAInternetURL", 8192, this, 1, NULL);
    }
    return void();
}

void OTAHelper::UpdateOTAInternetURL(void *pvParameters)
{

    OTAHelper *instance = (OTAHelper *)pvParameters;
    instance->_update_success = false;
    String updateURL = instance->_update_url;
    if (WiFi.status() != WL_CONNECTED)
    {
        ESP_LOGE(TAG, "No WiFi connection. OTA update aborted.");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Starting OTA update...");

    HTTPClient httpClient;
    httpClient.begin(updateURL);
    int httpCode = httpClient.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        int contentLength = httpClient.getSize();
        bool canBegin = Update.begin(contentLength);

        if (canBegin)
        {
            WiFiClient *client = httpClient.getStreamPtr();
            size_t written = Update.writeStream(*client);

            if (written == contentLength)
            {
                instance->_update_msg += "Written: " + String(written) + " bytes successfully";
                ESP_LOGI(TAG, "Written: %u successfully", written);
            }
            else
            {
                instance->_error_msg += "Written: " + String(written) + " out of " + String(contentLength) + " bytes only";
                instance->_error_msg += "Update failed";
                ESP_LOGE(TAG, "Written only: %u/%u. Update failed.", written, contentLength);
                return;
            }

            if (Update.end())
            {
                if (Update.isFinished())
                {
                    instance->_update_msg += "<br>Update successfully completed. Reboot to apply changes.";
                    ESP_LOGI(TAG, "Update successfully completed. Reboot to apply changes.");
                    instance->_update_success = true;
                }
                else
                {
                    instance->_error_msg += "Update not finished. Something went wrong!";
                    ESP_LOGE(TAG, "Update not finished. Something went wrong!");
                }
            }
            else
            {
                instance->_error_msg = "OTA update failed. Error #: " + String(Update.getError());
                ESP_LOGE(TAG, "Update failed. Error #: %d", Update.getError());
                instance->_error_msg = "Firmware download failed, HTTP response: " + Update.getError();
            }
        }
        else
        {
            ESP_LOGE(TAG, "Not enough space to begin OTA update");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Firmware download failed, HTTP response: %d", httpCode);
        instance->_error_msg = "Firmware download failed, HTTP response: " + String(httpCode);
    }

    httpClient.end();
    vTaskDelete(NULL);
}
