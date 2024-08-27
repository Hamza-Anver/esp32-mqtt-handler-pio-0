#include "configwebpage.h"

static const char *TAG = "ConfigWebpage";

#include <metaheaders.h>

#include CONFIG_WEBPAGE_GZIP_HEADER
#include CONFIG_KEYS_HEADER
#include ENDPOINTS_HEADER

/* -------------------------------------------------------------------------- */
/*                        MAIN CONFIG WEBPAGE FUNCTIONS                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Construct a new Config Webpage:: Config Webpage object
 *
 * @param config_helper optional argument to pass in a config helper object
 * @param ota_helper optional argument to pass in an ota helper object
 *
 *  If either or both are not passed, new ones will be created
 */
ConfigWebpage::ConfigWebpage(ConfigHelper *config_helper, OTAHelper *ota_helper)
{
    _server = new AsyncWebServer(80);
    _event_source = new AsyncEventSource(SERVER_SIDE_EVENTS_ENDPOINT);
    _server->addHandler(_event_source);
    _dnsServer = new DNSServer();
    _ap_ip = IPAddress(192, 168, 4, 1);
    _net_msk = IPAddress(255, 255, 255, 0);

    if (config_helper == nullptr)
    {
        _config_helper = new ConfigHelper(false);
    }
    else
    {
        _config_helper = config_helper;
    }

    if (ota_helper == nullptr)
    { // TODO: fix this assignment
        _ota_helper = nullptr;
    }
    else
    {
        _ota_helper = ota_helper;
    }

    // Start the access point
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(_ap_ip, _ap_ip, _net_msk);

    if (strlen(_config_helper->getConfigOption(ACCESSPOINT_PASS_KEY, "").c_str()) < 8)
    {
        WiFi.softAP(_config_helper->getConfigOption(ACCESSPOINT_SSID_KEY, "").c_str());
    }
    else
    {
        WiFi.softAP(_config_helper->getConfigOption(ACCESSPOINT_SSID_KEY, "").c_str(),
                    _config_helper->getConfigOption(ACCESSPOINT_PASS_KEY, "").c_str());
    }

    // Wait a second for AP to AP
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Set up DNS
    _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer->start(_dns_port, "*", _ap_ip);

    // Start server task (dns loop)
    xTaskCreate(ConfigWebpage::ConfigServerTask, "ConfigServerTask", 4096, this, 1, NULL);
    // Start OTA status task, will send updates if available
    xTaskCreate(ConfigWebpage::handleUpdateOTAStatus, "OTAStatusTask", 4096, this, 1, NULL);

    // Start captive portal callbacks
    _server->on("/", [this](AsyncWebServerRequest *request)
                { this->ConfigServeRootPage(request); });

    _server->on("/generate_204", [this](AsyncWebServerRequest *request)
                { this->ConfigServeRootPage(request); });

    _server->on("/fwlink", [this](AsyncWebServerRequest *request)
                { this->ConfigServeRootPage(request); });

    _server->onNotFound([this](AsyncWebServerRequest *request)
                        { this->ConfigServeRootPage(request); });

    // Device settings
    _server->on(DEVICE_SET_SETTINGS_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleDeviceSettingsConfig(request); });

    // Internet preferences
    _server->on(INTERNET_PREF_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleReceiveInternetPreferences(request); });

    // Station callbacks
    _server->on(STA_START_SCAN_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationStartScan(request); });
    _server->on(STA_SCAN_RESULTS_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationScanResults(request); });
    _server->on(STA_SET_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationSetConfig(request); });
    _server->on(STA_SEND_UPDATE_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationSendUpdate(request); });

    // LTE callbacks
    _server->on(LTE_SET_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleLTESetConfig(request); });

    // Access Point callbacks
    _server->on(AP_SET_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleAccessPointSetConfig(request); });

    // MQTT Callbacks
    _server->on(MQTT_SET_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleMQTTSetConfig(request); });

    // Config meta callbacks
    _server->on(CONFIG_JSON_GET_ENDPOINT, HTTP_GET, [this](AsyncWebServerRequest *request)
                { this->handleSendCurrentConfigJSON(request); });

    // System Status
    _server->on(DEVICE_GET_STATUS_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleSendStatusJSON(request); });

    // System callbacks
    _server->on(FACTORY_RESET_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleFactoryReset(request); });

    _server->on(RESTART_DEVICE_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleDeviceRestart(request); });

    // OTA Callbacks
    _server->on(UPDATE_OTA_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleUpdateOTAConfig(request); });

    _server->on(UPDATE_FIRMWARE_UPLOAD_ENDPOINT, HTTP_POST, [this](AsyncWebServerRequest *request)
                { request->send(200); }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
                { this->handleUpdateOTAFileUpload(request, filename, index, data, len, final); });

    _server->on(UPDATE_OTA_BIN_URL_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleUpdateOTANowRequest(request); });

    _server->on(UPDATE_OTA_STATUS_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleUpdateOTAStatus(request); });

    // TODO: remove links when killing server
    // TODO: Updating via file upload

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    // Begin the server
    _server->begin();

    ESP_LOGI(TAG, "Started the webserver");
}

/**
 * @brief TaskManager task to handle DNS requests
 *
 * @param pvParameters
 */
void ConfigWebpage::ConfigServerTask(void *pvParameters)
{
    // TODO: kill when done
    ConfigWebpage *instance = (ConfigWebpage *)pvParameters;
    for (;;)
    {
        instance->_dnsServer->processNextRequest();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Serves the root page from PROGMEM
 * 
 * This function can also be where authentication is implemented
 * 
 * @param request 
 */
void ConfigWebpage::ConfigServeRootPage(AsyncWebServerRequest *request)
{
    // if (!request->authenticate(_config_helper->getConfigOption(CONFIG_PORTAL_USERNAME_KEY, "admin").c_str(),
    //                            _config_helper->getConfigOption(CONFIG_PORTAL_PASSWORD_KEY, "admin").c_str()))
    //     return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_GZ_PROGMEM, HTML_GZ_PROGMEM_LEN);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

/* ----------------------------- DEVICE SETTINGS ---------------------------- */
/**
 * @brief NOT YET IMPLEMENTED - NOT FUNCTIONAL
 * 
 * This function is a placeholder for device settings configuration. 
 * The complete version could handle different user accounts, along with device names
 * and other suitable parameters.
 * 
 * @param request 
 */
void ConfigWebpage::handleDeviceSettingsConfig(AsyncWebServerRequest *request)
{

    JsonDocument responsedoc;

    String updatemsg = "<p class='updatebad'> Device settings not yet implemented!!! </p>";

    responsedoc["updatemsg"] = updatemsg;

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* -------------------------- CONFIG META FUNCTIONS ------------------------- */

/**
 * @brief Sends the current configuration JSON as a string to the webpage
 * 
 * This is part of a set of settings to make it easier to bulk view and set configurations
 *
 * @param request
 */
void ConfigWebpage::handleSendCurrentConfigJSON(AsyncWebServerRequest *request)
{

    String jsonString = _config_helper->getConfigJSONString();
    ESP_LOGI(TAG, "Config JSON: %s", jsonString.c_str());
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/**
 * @brief NOT YET IMPLEMENTED - NOT FUNCTIONAL
 * 
 * Receives a new JSON config as a string for bulk assignment. 
 * Should take in a JSON string validate it and then save it to the NVS
 *
 * @param request
 */
void ConfigWebpage::handleReceiveConfigJSON(AsyncWebServerRequest *request)
{
    // TODO: Receive JSON and save to NVS to make rapid config possible
}

/**
 * @brief Sends the current status of the device as a JSON string
 * 
 * Sends status updates to the webpage client with the current device parameters
 * The corresponding JS will update the status page with any number of parameters
 *
 * @param request
 */
void ConfigWebpage::handleSendStatusJSON(AsyncWebServerRequest *request)
{

    // WiFi status
    String message;
    String css_class;
    switch (WiFi.status())
    {
    case WL_CONNECTED:
        message = "Connected: " + String(WiFi.SSID());
        css_class = "infogood";
        break;
    case WL_NO_SSID_AVAIL:
        message = "No SSID Available";
        css_class = "infobad";
        break;
    case WL_CONNECT_FAILED:
        message = "Connection Failed";
        css_class = "infobad";
        break;
    case WL_IDLE_STATUS:
        message = "Idle";
        css_class = "infomeh";
        break;
    case WL_DISCONNECTED:
        message = "Disconnected";
        css_class = "infobad";
        break;
    default:
        message = "Unknown";
        css_class = "infobad";
        break;
    }

    _update_json["WiFi Status"]["msg"] = message;
    _update_json["WiFi Status"]["class"] = css_class;

    // IP Address
    message = String(WiFi.localIP().toString());
    _update_json["Device IP"] = message;

    // Uptime
    message = String(esp_timer_get_time() / 1000000) + " seconds";
    _update_json["Uptime"] = message;

    // Device heap
    _update_json["Free Heap"] = ESP.getFreeHeap();

    // Max alloc heap
    _update_json["Max Free Heap"] = ESP.getMaxAllocHeap();

    String jsonString;
    ArduinoJson::serializeJsonPretty(_update_json, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);

    ESP_LOGI(TAG, "Sending status JSON [%s]", jsonString.c_str());
}

/* -------------------------------------------------------------------------- */
/*                              SETTINGS FUNCTION                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Handles factory reset, by calling the appropriate config helper functions
 *
 * @param request
 */
void ConfigWebpage::handleFactoryReset(AsyncWebServerRequest *request)
{
    request->send(200);
    ESP_LOGI("ConfigHandler", "Factory reset requested from webpage");
    _config_helper->restoreDefaultConfigJSON(true);
    
    _event_source->send("Device has been reset, restart to complete", "update");
}

/**
 * @brief Restarts device, may result in client disconnection
 *
 * @param request
 */
void ConfigWebpage::handleDeviceRestart(AsyncWebServerRequest *request)
{
    // TODO: reload any clients views of the page so that they have the latest version
    
    request->send(200);
    ESP_LOGI("ConfigHandler", "Restart requested from webpage");
    JsonDocument responsedoc;
    _event_source->send("Restarting now", "update");
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP.restart();
}
/* -------------------------------------------------------------------------- */
/*                               INTERNET ACCESS                              */
/* -------------------------------------------------------------------------- */
/* -------------------------- INTERNET PREFERENCES -------------------------- */
/**
 * @brief Receive, validate and save internet preferences
 *
 * @param request
 */
void ConfigWebpage::handleReceiveInternetPreferences(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI(TAG, "Callback Received [Params: %d]", params);
    String interneturl = "";
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI(TAG, "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), NET_PRIORITY_PREF_KEY) == 0)
        {
            _config_helper->setConfigOption(NET_PRIORITY_PREF_KEY, p->value().c_str());
        }
        // TODO: switch time parameter
    }

    JsonDocument responsedoc;

    String updatemsg = "";
    updatemsg += "<p class='updategood'> Internet Preferences have been saved! [" +
                 String(_config_helper->getConfigOption(NET_PRIORITY_PREF_KEY, "ERROR")) + "] </p>";

    responsedoc["updatemsg"] = updatemsg;

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* ------------------------------ LTE FUNCTIONS ----------------------------- */

/**
 * @brief Receive validate and set LTE settings
 *
 * @param request
 */
void ConfigWebpage::handleLTESetConfig(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI(TAG, "Callback Received [Params: %d]", params);
    String lteapn = "";
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI(TAG, "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), A76XX_APN_NAME_KEY) == 0)
        {
            lteapn = p->value();
        }
    }

    // Validate APN
    String err_msg = "";

    if (lteapn.length() == 0)
    {
        err_msg += "APN field is empty <br>";
    }
    for (int i = 0; i < lteapn.length(); i++)
    {
        char c = lteapn.charAt(i);
        if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
        {
            err_msg += "APN has unallowed char(s) <br>";
            break;
        }
    }

    JsonDocument responsedoc;

    String updatemsg = "";
    if (err_msg.length() > 0)
    {
        updatemsg += "<p class='updatebad'> " + err_msg + "<br> LTE settings not saved </p>";
    }
    else
    {
        _config_helper->setConfigOption(A76XX_APN_NAME_KEY, lteapn.c_str());
        updatemsg += "<p class='updategood'> LTE Preferences have been saved! </p>";
    }

    responsedoc["updatemsg"] = updatemsg;

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* ---------------------------- STATION FUNCTIONS --------------------------- */
/**
 * @brief Start scanning for WiFi networks asynchronously when called
 *
 * @param request
 */
void ConfigWebpage::handleStationStartScan(AsyncWebServerRequest *request)
{
    ESP_LOGI(TAG, "Scanning for WiFi networks...");
    // Start Asynchronous scan for WiFi networks
    WiFi.scanNetworks(true);
    // Get the number of networks found
    int num_networks = WiFi.scanComplete();
    // Send the number of networks found
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("YEET");

    request->send(response);
}

/**
 * @brief Sends back the num_networks found and the networks in a JSON array
 *
 * The page JS will disregard if there is only one element (i.e. no networks to show)
 * Will keep being called till there are networks found
 *
 * @param request
 */
void ConfigWebpage::handleStationScanResults(AsyncWebServerRequest *request)
{
    // Get the number of networks found
    int num_networks = WiFi.scanComplete();
    ESP_LOGI(TAG, "Callback Received: Returning [%d] networks", num_networks);
    // Send the number of networks found
    JsonDocument jsonDoc;
    JsonArray networks = jsonDoc.to<JsonArray>();
    JsonObject net_stat = networks.add<JsonObject>();
    net_stat["num_networks"] = num_networks;

    for (int i = 0; i < num_networks; i++)
    {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encryption"] = WiFi.encryptionType(i);
    }
    // Reset if scan is done and results sent
    // Avoids polluting with old results
    if (num_networks > 0)
    {
        WiFi.scanDelete();
    }

    String jsonString;
    ArduinoJson::serializeJson(jsonDoc, jsonString);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString);

    request->send(response);
}

/**
 * @brief Set the station configuration and attempt to connect
 *
 *
 * @param request
 */
void ConfigWebpage::handleStationSetConfig(AsyncWebServerRequest *request)
{
    _sta_attempts = 0;
    int params = request->params();
    ESP_LOGI(TAG, "Callback Received [Params: %d]", params);

    _sta_ssid = "";
    _sta_pass = "";

    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI(TAG, "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), STATION_SSID_KEY) == 0)
        {
            _sta_ssid = p->value();
        }
        else if (strcmp(p->name().c_str(), STATION_PASS_KEY) == 0)
        {
            _sta_pass = p->value();
        }
    }

    // TODO: handle multiple credentials

    String err_msg = "";

    if (_sta_ssid.length() < 1 || _sta_ssid.length() > 32)
    {
        err_msg += "SSID is not the right length <br>";
    }

    // Check if all characters are valid (alphanumeric or allowed special characters)
    for (int i = 0; i < _sta_ssid.length(); i++)
    {
        char c = _sta_ssid.charAt(i);
        if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
        {
            err_msg += "SSID has unallowed character(s) <br>";
            break;
        }
    }

    request->send(200);
    if (err_msg.length() == 0)
    {
        WiFi.disconnect();
        if (_sta_pass.length() == 0)
        {
            WiFi.begin(_sta_ssid.c_str());
        }
        else
        {
            WiFi.begin(_sta_ssid.c_str(), _sta_pass.c_str());
        }

        String update_msg = "Trying to connect to '" + _sta_ssid + "'";
        _event_source->send(update_msg.c_str(), "update");
    }
    else
    {
        String update_msg = err_msg + "<br> Credentials not saved";
        _event_source->send(update_msg.c_str(), "updatebad");
    }

    xTaskCreate(ConfigWebpage::handleStationSendUpdate, "staConnect", 4096, this, 1, NULL);
}

/**
 * @brief Attempt to connect to the WiFi network
 *
 * Sends updates for number of attempts while trying. Failure message and success message
 *
 * Saves SSID and Password if successful only
 *
 * @param request
 */
void ConfigWebpage::handleStationSendUpdate(void *pvParameters)
{
    ConfigWebpage *instance = (ConfigWebpage *)pvParameters;
    int sta_attempts = 0;
    while (true)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            String update_msg = "Connected to '" + instance->_sta_ssid + "'!  <br> Credentials Saved!";
            instance->_event_source->send(update_msg.c_str(), "updategood");
            instance->_config_helper->setConfigOption(STATION_SSID_KEY, instance->_sta_ssid.c_str());
            instance->_config_helper->setConfigOption(STATION_PASS_KEY, instance->_sta_pass.c_str());
            break;
        }
        else
        {
            sta_attempts++;
            String update_msg = "Connecting.. attempt [" + String(sta_attempts) + "/5]";
            instance->_event_source->send(update_msg.c_str(), "updatebad");
        }

        if (sta_attempts > 5)
        {
            // Avoid constant reconnection attempts
            WiFi.disconnect();
            String update_msg = "Failed to connect to '" + instance->_sta_ssid + "' :(  <br> Credentials not saved";
            instance->_event_source->send(update_msg.c_str(), "updatebad");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

/* -------------------------------------------------------------------------- */
/*                            MQTT CONFIG FUNCTION                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Receive and validate the data from the MQTT form
 *
 * There is some fuckery going on to deal with booleans nicely
 *
 * Options will only be saved if there are 0 errors logged
 * Can still result in empty options
 *
 * @param request
 */
void ConfigWebpage::handleMQTTSetConfig(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI(TAG, "Callback Received [Params: %d]", params);

    // Temporary variables to store arguments
    // This is not the most efficient way to do this
    // but it'll work for now
    String mqtt_client_id = "";
    String mqtt_username = "";
    String mqtt_password = "";
    String mqtt_server_url = "";
    int mqtt_server_port = 0;
    int mqtt_keepalive = 0;
    bool mqtt_clean_session = false;
    bool mqtt_retain = false;
    String mqtt_lwt_topic = "";
    String mqtt_lwt_payload = "";
    int mqtt_lwt_qos = 0;

    String response_msgs = "";
    String err_msgs = "";

    bool errordet = false;

    for (int i = 0; i < params; i++)
    {
        errordet = false;
        AsyncWebParameter *p = request->getParam(i);

        ESP_LOGI(TAG, "POST[%s]: %s", p->name().c_str(), p->value().c_str());

        if (strcmp(p->name().c_str(), MQTT_CLIENT_ID_KEY) == 0)
        {

            // Check MQTT Client ID
            mqtt_client_id = p->value();

            // Add UID if pattern match
            if (mqtt_client_id != "" && mqtt_client_id.length() < 256)
            {
                // TODO: case insensitve replacement
                mqtt_client_id.replace(UID_REPLACEMENT_PATTERN, _config_helper->getDeviceMACString());
            }
            for (int i = 0; i < mqtt_client_id.length(); i++)
            {
                char c = mqtt_client_id.charAt(i);
                if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
                {
                    err_msgs += "MQTT Client ID has unallowed char(s) <br>";
                    errordet = true;
                    break;
                }
            }

            if (!errordet)
            {
                response_msgs += "MQTT Client ID [" + mqtt_client_id + "] saved <br>";
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_USERNAME_KEY) == 0)
        {
            mqtt_username = p->value();
            for (int i = 0; i < mqtt_username.length(); i++)
            {
                char c = mqtt_username.charAt(i);
                if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
                {
                    err_msgs += "MQTT Username has unallowed char(s) <br>";
                    errordet = true;
                    break;
                }
            }

            if (!errordet)
            {
                if (mqtt_username.length() == 0)
                {
                    response_msgs += "MQTT username left empty <br>";
                }
                else
                {
                    response_msgs += "MQTT username [" + mqtt_username + "] saved <br>";
                }
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_PASSWORD_KEY) == 0)
        {
            mqtt_password = p->value();

            if (!errordet)
            {
                if (mqtt_username.length() == 0)
                {
                    response_msgs += "MQTT password left empty <br>";
                }
                else
                {
                    response_msgs += "MQTT password saved <br>";
                }
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_SERVER_URL_KEY) == 0)
        {
            mqtt_server_url = p->value();

            if (mqtt_server_url.length() == 0)
            {
                err_msgs += "MQTT Server URL left empty <br>";
            }
            else if (mqtt_server_url.length() > 256)
            {
                err_msgs += "MQTT Server URL too long <br>";
            }
            else if (mqtt_server_url.indexOf(" ") >= 0)
            {
                err_msgs += "MQTT Server URL contains spaces <br>";
            }
            else
            {
                response_msgs += "MQTT Server URL [" + mqtt_server_url + "] saved <br>";
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_SERVER_PORT_KEY) == 0)
        {
            mqtt_server_port = p->value().toInt();
            if (mqtt_server_port < 1 || mqtt_server_port > 65535)
            {
                err_msgs += "MQTT Server Port is not between 1 and 65535 <br>";
            }
            else
            {
                response_msgs += "MQTT Server Port [" + String(mqtt_server_port) + "] saved <br>";
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_KEEPALIVE_KEY) == 0)
        {
            mqtt_keepalive = p->value().toInt();

            if (mqtt_keepalive < 0)
            {
                err_msgs += "MQTT Keepalive is negative";
            }
            else
            {
                if (mqtt_keepalive == 0)
                {
                    response_msgs += "MQTT Keepalive disabled <br>";
                }
                else
                {
                    response_msgs += "MQTT Keepalive [" + String(mqtt_keepalive) + "] saved <br>";
                }
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_CLEAN_SESSION_KEY) == 0)
        {
            mqtt_clean_session = (strcmp(p->value().c_str(), MQTT_CLEAN_SESSION_TRUE_OPTION) == 0);
            response_msgs += "MQTT Clean Session [" + String(mqtt_clean_session) + "] saved <br>";
        }
        else if (strcmp(p->name().c_str(), MQTT_RETAIN_KEY) == 0)
        {
            mqtt_retain = (strcmp(p->value().c_str(), MQTT_RETAIN_TRUE_OPTION) == 0);
            response_msgs += "MQTT Retain [" + String(mqtt_retain) + "] saved <br>";
        }
        else if (strcmp(p->name().c_str(), MQTT_LWT_TOPIC_KEY) == 0)
        {
            mqtt_lwt_topic = p->value();
            if (mqtt_lwt_topic.length() == 0)
            {
                response_msgs += "MQTT LWT Topic left empty <br>";
            }
            else
            {
                response_msgs += "MQTT LWT Topic [" + mqtt_lwt_topic + "] saved <br>";
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_LWT_PAYLOAD_KEY) == 0)
        {
            mqtt_lwt_payload = p->value();
            if (mqtt_lwt_payload.length() == 0)
            {
                response_msgs += "MQTT LWT Payload left empty <br>";
            }
            else
            {
                response_msgs += "MQTT LWT Payload [" + mqtt_lwt_payload + "]saved <br>";
            }
        }
        else if (strcmp(p->name().c_str(), MQTT_LWT_QOS_KEY) == 0)
        {
            mqtt_lwt_qos = p->value().toInt();
            if (mqtt_lwt_qos < 0 || mqtt_lwt_qos > 2)
            {
                err_msgs += "MQTT LWT QoS is not between 0 and 2 <br>";
            }
            else
            {
                response_msgs += "MQTT LWT QoS [" + String(mqtt_lwt_qos) + "] saved <br>";
            }
        }
    }

    request->send(200);

    if (err_msgs.length() == 0)
    {
        // i.e. no errors
        _config_helper->setConfigOption(MQTT_CLIENT_ID_KEY, mqtt_client_id.c_str());
        _config_helper->setConfigOption(MQTT_USERNAME_KEY, mqtt_username.c_str());
        _config_helper->setConfigOption(MQTT_PASSWORD_KEY, mqtt_password.c_str());
        _config_helper->setConfigOption(MQTT_SERVER_URL_KEY, mqtt_server_url.c_str());
        _config_helper->setConfigOption(MQTT_SERVER_PORT_KEY, mqtt_server_port);
        _config_helper->setConfigOption(MQTT_KEEPALIVE_KEY, mqtt_keepalive);
        _config_helper->setConfigOption(MQTT_CLEAN_SESSION_KEY, mqtt_clean_session);
        _config_helper->setConfigOption(MQTT_RETAIN_KEY, mqtt_retain);
        _config_helper->setConfigOption(MQTT_LWT_TOPIC_KEY, mqtt_lwt_topic.c_str());
        _config_helper->setConfigOption(MQTT_LWT_PAYLOAD_KEY, mqtt_lwt_payload.c_str());
        _config_helper->setConfigOption(MQTT_LWT_QOS_KEY, mqtt_lwt_qos);
        _event_source->send(response_msgs.c_str(), "updategood");
    }
    else
    {
        err_msgs += "<br> MQTT Config not saved :(";
        _event_source->send(err_msgs.c_str(), "updatebad");
    }
}

/* -------------------------------------------------------------------------- */
/*                        ACCESS POINT CONFIG FUNCTION                        */
/* -------------------------------------------------------------------------- */

/**
 * @brief Receive, validate and save the access point settings
 *
 * Formats the SSID with the UID if the pattern is found
 *
 * @param request
 */
void ConfigWebpage::handleAccessPointSetConfig(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("StationConfig", "Callback Received [Params: %d]", params);
    String apssid = "";
    String appass = "";
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("StationConfig", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), ACCESSPOINT_SSID_KEY) == 0)
        {
            apssid = p->value();

            // Add UID if pattern match
            if (apssid != "" && apssid.length() < 32)
            {
                // TODO: case insensitve replacement
                apssid.replace(UID_REPLACEMENT_PATTERN, _config_helper->getDeviceMACString());
            }
        }
        else if (strcmp(p->name().c_str(), ACCESSPOINT_PASS_KEY) == 0)
        {
            appass = p->value();
        }
    }

    request->send(200);

    String err_msgs = "";

    if (apssid.length() < 1 || apssid.length() > 32)
    {
        err_msgs += "SSID is not the right length <br>";
    }

    // Check if all characters are valid (alphanumeric or allowed special characters)
    for (int i = 0; i < apssid.length(); i++)
    {
        char c = apssid.charAt(i);
        if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
        {
            err_msgs += "SSID has unallowed character(s) <br>";
            break;
        }
    }

    if ((appass.length() < 8 || appass.length() > 63) && appass.length() != 0)
    {
        err_msgs += "Password is not 0 or between 8 and 64 (inclusive) characters long <br>";
    }

    // Check if all characters are valid printable ASCII characters
    for (int i = 0; i < appass.length(); i++)
    {
        char c = appass.charAt(i);
        if (c < 32 || c > 126)
        { // ASCII 32 to 126 are printable characters
            err_msgs += "Password contains unprintable characters <br>";
        }
    }
    JsonDocument responsedoc;
    if (err_msgs != "")
    {
        // i.e. validation failed
        _event_source->send(err_msgs.c_str(), "updatebad");
    }
    else
    {
        String updatemsg = "<p class='updategood'> Credentials with SSID: " + apssid + " have been saved!</p>";
        _event_source->send(updatemsg.c_str(), "updategood");
        // Save to flash here
        _config_helper->setConfigOption("apssid", apssid.c_str());
        _config_helper->setConfigOption("appass", appass.c_str());
    }
}

/* -------------------------------------------------------------------------- */
/*                                OTA FUNCTIONS                               */
/* -------------------------------------------------------------------------- */

void ConfigWebpage::handleUpdateOTAConfig(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("OTAUpdate", "Callback Received [Params: %d]", params);
    String updateURL = "";
    int check_freq = 0;
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("OTAUpdate", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), OTA_JSON_URL_KEY) == 0)
        {
            updateURL = p->value();
        }
        else if (strcmp(p->name().c_str(), OTA_UPDATE_FREQUENCY_KEY) == 0)
        {
            check_freq = p->value().toInt();
        }
    }
    request->send(200);

    String update_msg = "";
    if (updateURL.length() > 4)
    {
        update_msg = "Saving [" + updateURL +"] with frequency " + String(check_freq) + " minutes";
        _event_source->send(update_msg.c_str(), "updategood");
        if(_ota_helper->CheckUpdateJSON(updateURL)){
            if(_ota_helper->CheckJSONForNewVersion()){
                _event_source->send("New firmware available, downloading now!", "updategood");
                _ota_helper->CallOTAInternetUpdateAsync();
            }else{
                _event_source->send("No new firmware available", "update");

            }
        } else{
            _event_source->send("Checking JSON failed", "updatebad");
        }
    }
    
}

/**
 * @brief Sends the current status of the OTA update
 *
 * Formats a string to have percentage and size of update
 *
 * @param request
 */
void ConfigWebpage::handleUpdateOTAStatus(void *pvParameters)
{
    ConfigWebpage *instance = (ConfigWebpage *)pvParameters;
    String update_msg = "";
    bool retcode = false;
    for (;;)
    {
        if (instance->_ota_helper->CheckOTAHasMessage())
        {
            retcode = instance->_ota_helper->GetOTAMessage(update_msg);
            if (retcode)
            {
                instance->_event_source->send(update_msg.c_str(), "updategood");
            }
            else
            {
                instance->_event_source->send(update_msg.c_str(), "updatebad");
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Handle receiving a file upload for OTA update
 *
 * @param request
 */
void ConfigWebpage::handleUpdateOTAFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    _ota_helper->handleOTAChunks(filename, index, data, len, final, request->contentLength());
}

/**
 * @brief Immediately check the URL for an OTA update
 *
 * @param request
 */
void ConfigWebpage::handleUpdateOTANowRequest(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("OTAUpdate", "Callback Received [Params: %d]", params);
    String updateURL = "";
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("OTAUpdate", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), OTA_BIN_URL_KEY) == 0)
        {
            updateURL = p->value();
        }
    }

    request->send(200);

    String update_msg = "Checking: " + updateURL;
    _event_source->send(update_msg.c_str(), "update");

    if (updateURL != "")
    {
        _ota_helper->CallOTAInternetUpdateAsync(updateURL);
    }
}