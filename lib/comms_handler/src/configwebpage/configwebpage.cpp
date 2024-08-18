#include "configwebpage.h"

static const char *TAG = "ConfigWebpage";

#include <metaheaders.h>

#include CONFIG_WEBPAGE_GZIP_HEADER
#include CONFIG_KEYS_HEADER
#include ENDPOINTS_HEADER

/* -------------------------------------------------------------------------- */
/*                        MAIN CONFIG WEBPAGE FUNCTIONS                       */
/* -------------------------------------------------------------------------- */

ConfigWebpage::ConfigWebpage(ConfigHelper *config_helper, OTAHelper *ota_helper)
{
    _server = new AsyncWebServer(80);
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
    {
        _ota_helper = new OTAHelper(_config_helper);
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

    // Start captive portal callbacks
    _server->on("/", [this](AsyncWebServerRequest *request)
                { this->ConfigServeRootPage(request); });

    _server->on("/generate_204", [this](AsyncWebServerRequest *request)
                { this->ConfigServeRootPage(request); });

    _server->on("/fwlink", [this](AsyncWebServerRequest *request)
                { this->ConfigServeRootPage(request); });

    _server->onNotFound([this](AsyncWebServerRequest *request)
                        { this->ConfigServeRootPage(request); });

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
    _server->on(UPDATE_OTA_BIN_URL_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleUpdateOTANowRequest(request); });

    _server->on(UPDATE_OTA_STATUS_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleUpdateOTAStatus(request); });

    // TODO: Updating via file upload

    // Begin the server
    _server->begin();

    ESP_LOGI(TAG, "Started the webserver");
}

void ConfigWebpage::ConfigServerTask(void *pvParameters)
{
    ConfigWebpage *instance = (ConfigWebpage *)pvParameters;
    for (;;)
    {
        instance->_dnsServer->processNextRequest();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ConfigWebpage::ConfigServeRootPage(AsyncWebServerRequest *request)
{
    // TODO: add password auth here
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_GZ_PROGMEM, HTML_GZ_PROGMEM_LEN);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}

/* -------------------------- CONFIG META FUNCTIONS ------------------------- */
void ConfigWebpage::handleSendCurrentConfigJSON(AsyncWebServerRequest *request)
{

    String jsonString = _config_helper->getConfigJSONString();
    ESP_LOGI(TAG, "Config JSON: %s", jsonString.c_str());
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

void ConfigWebpage::handleReceiveConfigJSON(AsyncWebServerRequest *request)
{
    // TODO: Receive JSON and save to NVS to make rapid config possible
}

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

    _update_json[WIFI_STATUS_MSG_ID]["msg"] = message;
    _update_json[WIFI_STATUS_MSG_ID]["class"] = css_class;

    // IP Address
    message = String(WiFi.localIP().toString());
    _update_json[DEVICE_IP_MSG_ID] = message;

    // Uptime
    message = String(esp_timer_get_time() / 1000000) + " seconds";
    _update_json[DEVICE_UPTIME_MSG_ID] = message;

    // Device heap
    _update_json[DEVICE_FREE_HEAP_MSG_ID] = ESP.getFreeHeap();

    // Max alloc heap
    _update_json[DEVICE_MAX_ALLOC_HEAP_MSG_ID] = ESP.getMaxAllocHeap();

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

void ConfigWebpage::handleFactoryReset(AsyncWebServerRequest *request)
{
    ESP_LOGI("ConfigHandler", "Factory reset requested from webpage");
    _config_helper->restoreDefaultConfigJSON(true);
    JsonDocument responsedoc;
    responsedoc["updatemsg"] = "<p class='updategood'> Configuration has been reset! </p>";
    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
    ESP_LOGI(TAG, "Factory reset done from webpage");
}

void ConfigWebpage::handleDeviceRestart(AsyncWebServerRequest *request)
{
    ESP_LOGI("ConfigHandler", "Restart requested from webpage");
    JsonDocument responsedoc;
    responsedoc["updatemsg"] = "<p class='update'> Restarting </p>";
    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP.restart();
}
/* -------------------------------------------------------------------------- */
/*                               INTERNET ACCESS                              */
/* -------------------------------------------------------------------------- */

/* -------------------------- INTERNET PREFERENCES -------------------------- */
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

    // TODO: Validate SSID and Password before attempting connection and saving

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

    JsonDocument responsedoc;
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

        responsedoc["endpoint"] = STA_SEND_UPDATE_ENDPOINT;
        String update_msg = "<p class='update'> Trying to connect to '" + _sta_ssid + "' </p>";
        responsedoc["updatemsg"] = update_msg;
        responsedoc["timeout"] = "1000";
    }
    else
    {
        responsedoc["updatemsg"] = "<p class='updatebad'> " + err_msg + "<br> Credentials not saved </p>";
    }

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

void ConfigWebpage::handleStationSendUpdate(AsyncWebServerRequest *request)
{
    if (_sta_ssid.length() == 0 || _sta_pass.length() == 0)
    {
        ESP_LOGE(TAG, "No SSID or Password set");
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected to WiFi");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
    }

    _sta_attempts++;
    JsonDocument responsedoc;

    ESP_LOGI(TAG, "Attempting connection with SSID [%s] and password [%s]",
             _sta_ssid.c_str(),
             _sta_pass.c_str());

    WiFi.begin(_sta_ssid.c_str(), _sta_pass.c_str());
    if (WiFi.status() != WL_CONNECTED)
    {
        if (_sta_attempts > 5)
        {
            ESP_LOGE(TAG, "Failed to connect to WiFi");
            String update_msg = "<p class='updatebad'> Failed to connect to '" + _sta_ssid + "' :(  <br> Credentials not saved</p>";
            responsedoc["updatemsg"] = update_msg.c_str();
        }
        else
        {
            ESP_LOGI(TAG, "Failed to connect to WiFi, retrying");
            responsedoc["endpoint"] = STA_SEND_UPDATE_ENDPOINT;
            String updatemsg = "<p class='update'> Connecting.. attempt [" + String(_sta_attempts) + "/5] </p>";
            responsedoc["updatemsg"] = updatemsg;
            responsedoc["timeout"] = "1000";
        }
    }
    else
    {
        String update_msg = "<p class='updategood'> Connected to '" + _sta_ssid + "'!  <br> Credentials Saved!</p>";
        responsedoc["updatemsg"] = update_msg.c_str();
        _config_helper->setConfigOption(STATION_SSID_KEY, _sta_ssid.c_str());
        _config_helper->setConfigOption(STATION_PASS_KEY, _sta_pass.c_str());
    }

    String jsonString = "";
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* -------------------------------------------------------------------------- */
/*                            MQTT CONFIG FUNCTION                            */
/* -------------------------------------------------------------------------- */

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

    JsonDocument responsedoc;

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
        responsedoc["updatemsg"] = "<p class='updategood'>" +
                                   response_msgs + "<br> MQTT Config has been saved! </p>";
    }
    else
    {
        responsedoc["updatemsg"] = "<p class='updatebad'>" +
                                   err_msgs + "<br> MQTT Config not saved :( </p>";
    }

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* -------------------------------------------------------------------------- */
/*                        ACCESS POINT CONFIG FUNCTION                        */
/* -------------------------------------------------------------------------- */
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

    String validationresponse = "";

    if (apssid.length() < 1 || apssid.length() > 32)
    {
        validationresponse += "SSID is not the right length <br>";
    }

    // Check if all characters are valid (alphanumeric or allowed special characters)
    for (int i = 0; i < apssid.length(); i++)
    {
        char c = apssid.charAt(i);
        if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
        {
            validationresponse += "SSID has unallowed character(s) <br>";
            break;
        }
    }

    if ((appass.length() < 8 || appass.length() > 63) && appass.length() != 0)
    {
        validationresponse += "Password is not 0 or between 8 and 64 (inclusive) characters long <br>";
    }

    // Check if all characters are valid printable ASCII characters
    for (int i = 0; i < appass.length(); i++)
    {
        char c = appass.charAt(i);
        if (c < 32 || c > 126)
        { // ASCII 32 to 126 are printable characters
            validationresponse += "Password contains unprintable characters <br>";
        }
    }
    JsonDocument responsedoc;
    if (validationresponse != "")
    {
        // i.e. validation failed
        validationresponse = "<p class='updatebad'>" + validationresponse + "</p>";
        responsedoc["updatemsg"] = validationresponse.c_str();
    }
    else
    {
        String updatemsg = "<p class='updategood'> Credentials with SSID: " + apssid + " have been saved!</p>";
        responsedoc["updatemsg"] = updatemsg;
        // Save to flash here
        _config_helper->setConfigOption("apssid", apssid.c_str());
        _config_helper->setConfigOption("appass", appass.c_str());
    }

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* -------------------------------------------------------------------------- */
/*                                OTA FUNCTIONS                               */
/* -------------------------------------------------------------------------- */

void ConfigWebpage::handleUpdateOTAStatus(AsyncWebServerRequest *request)
{
    JsonDocument responsedoc;
    String updateStatus = _ota_helper->GetOTAUpdateStatus();

    ESP_LOGI("OTAUpdate", "Status message: [%s]", updateStatus.c_str());

    responsedoc["updatemsg"] = updateStatus.c_str();
    if (_ota_helper->OTARunning())
    {
        responsedoc["endpoint"] = UPDATE_OTA_STATUS_ENDPOINT;
        responsedoc["timeout"] = "2000";
    }

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

void ConfigWebpage::handleUpdateOTAFileUpload(AsyncWebServerRequest *request)
{
}

void ConfigWebpage::handleUpdateOTANowRequest(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("OTAUpdate", "Callback Received [Params: %d]", params);
    String updateURL = "";
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("OTAUpdate", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), OTA_URL_KEY) == 0)
        {
            updateURL = p->value();
        }
    }

    JsonDocument responsedoc;
    if (updateURL != "")
    {
        responsedoc["updatemsg"] = "<p class='update'> Checking URL for update </p>";
        responsedoc["endpoint"] = UPDATE_OTA_STATUS_ENDPOINT;
        responsedoc["timeout"] = "5000";
    }
    else
    {
        responsedoc["updatemsg"] = "<p class='updatebad'> No URL provided </p>";
    }

    String jsonString;
    ArduinoJson::serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);

    if (updateURL != "")
    {
        _ota_helper->CallOTAInternetUpdateAsync(updateURL);
    }
}