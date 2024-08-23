#include "otahelper.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include <Update.h>

#include <version.h>

static const char *TAG = "OTAHelper";

// TODO: Filesystem updates
// TODO: JSON checking

OTAHelper::OTAHelper(A76XX *a76xx)
{
    _a76xx = a76xx;
    _update_msg.reserve(512);
    _error_msg.reserve(512);
}

bool OTAHelper::CheckOTAHasMessage()
{
    return (_update_msg != "" ||
            _error_msg != "" ||
            Update.isRunning());
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
        ESP_LOGI(TAG, "Message: %s", message.c_str());
        _update_msg = "";
        return true;
    }
    if (_error_msg != "")
    {
        message = _error_msg;
        ESP_LOGI(TAG, "Error: %s", message.c_str());
        _error_msg = "";
        return false;
    }
    if (Update.isRunning())
    {
        message = "Downloading [" + _update_url + "] " + String(Update.progress() * 100 / Update.size()) + " % [" + String(Update.progress()) + "/" + String(Update.size()) + "] bytes";
        ESP_LOGI(TAG, "Message: %s", message.c_str());
        return true;
    }
    return false;
}

void OTAHelper::ClearOTAMessageStrings()
{
    _error_msg = "";
    _update_msg = "";
}

void OTAHelper::AddToUpdateString(String update_msg)
{
    if (_update_msg.length() + update_msg.length() > 500)
    {
        _update_msg.clear();
        _update_msg = update_msg;
    }
    else
    {
        _update_msg += update_msg;
    }
}

void OTAHelper::AddToErrorString(String error_msg)
{
    if (_error_msg.length() + error_msg.length() > 500)
    {
        _error_msg.clear();
        _error_msg = error_msg;
    }
    else
    {
        _error_msg += error_msg;
    }
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
                    payload.replace("\n", "<br>");
                    _update_msg += "Received JSON: <br>" + payload;
                    _wifi_http->end();
                    return true;
                }
                else
                {
                    _error_msg = "JSON parsing from " + jsonurl + " failed!";
                    ESP_LOGE(TAG, "JSON parsing failed");
                    _wifi_http->end();
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
        }
        else
        {
            _error_msg += "Failed to start the WiFi HTTP Client with URL: [" + jsonurl + "]";
            _wifi_http->end();
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

bool OTAHelper::CheckJSONForNewVersion(bool str_msg)
{
    if (_update_meta_json.isNull() == false)
    {
        // There is a JSON file with data
        // TODO: establish some logic here
        if (_update_meta_json.containsKey("major") &&
            _update_meta_json.containsKey("minor") &&
            _update_meta_json.containsKey("patch") &&
            _update_meta_json.containsKey("binurl") &&
            _update_meta_json.containsKey("env"))
        {
            if (strcmp(VERSION_ENVIRONMENT, _update_meta_json["env"].as<String>().c_str()) == 0)
            {
                // Environment matches
                int new_major = _update_meta_json["major"];
                int new_minor = _update_meta_json["minor"];
                int new_patch = _update_meta_json["patch"];
                if (new_major > VERSION_MAJOR ||
                    new_minor > VERSION_MINOR ||
                    new_patch > VERSION_PATCH)
                {
                    // Version is newer
                    _update_url = _update_meta_json["binurl"].as<String>();
                    _update_msg = "New Version " + _update_meta_json["name"].as<String>() + " found!";
                    _update_msg += "<br> Downloading bin from: " + _update_url;
                    ESP_LOGI(TAG, "New firmware [%s] bin at: %s",
                             _update_meta_json["name"].as<String>().c_str(),
                             _update_url.c_str());
                    return true;
                }
                else
                {
                    _error_msg = "New version was not found from JSON file";
                    ESP_LOGE(TAG, "No new firmware found");
                    return false;
                }
            }
        }
    }

    return false;
}

void OTAHelper::handleOTAChunks(String filename,
                                size_t index,
                                uint8_t *data,
                                size_t len,
                                bool final,
                                size_t reqlen,
                                bool str_msg)
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

void OTAHelper::CallOTAInternetUpdateAsync(String updateURL,
                                           bool autorestart,
                                           bool str_msg)
{
    // TODO: Make this work with A76XX HTTP too
    // using a76xx http may be hard
    // Can use the MQTT stream to write chunks instead
    if (updateURL != "")
    {
        _update_url = updateURL;
    }

    _auto_restart = autorestart;

    // 4 is an arbitray number
    if (_update_url.length() < 4)
    {
        _error_msg += "Bin file URL is invalid [" + _update_url + "]";
    }
    else
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            ESP_LOGE(TAG, "No WiFi connection. OTA update aborted.");
            _error_msg += "No WiFi connection";
        }
        else
        {
            ESP_LOGI(TAG, "Starting asynchronous OTA update");
            xTaskCreate(UpdateOTAInternetURL, "UpdateOTAInternetURL", 8192, this, 1, NULL);
        }
    }
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
                    if (instance->_auto_restart)
                    {
                        ESP.restart();
                    }
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
            instance->_error_msg = "OTA update failed. Insufficient heap.";
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
