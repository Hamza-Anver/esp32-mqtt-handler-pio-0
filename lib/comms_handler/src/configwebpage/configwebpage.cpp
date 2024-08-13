#include "configwebpage.h"

static const char *TAG = "ConfigWebpage";

ConfigWebpage::ConfigWebpage(ConfigHelper *config_helper)
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

    // Station callbacks
    _server->on(STA_START_SCAN_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationStartScan(request); });
    _server->on(STA_SCAN_RESULTS_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationScanResults(request); });
    _server->on(STA_SET_CONFIG_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationSetConfig(request); });
    _server->on(STA_SEND_UPDATE_ENDPOINT, [this](AsyncWebServerRequest *request)
                { this->handleStationSendUpdate(request); });

    // Config meta callbacks
    _server->on(CONFIG_JSON_GET_ENDPOINT, HTTP_GET, [this](AsyncWebServerRequest *request)
                { this->handleSendCurrentConfigJSON(request); });

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
    serializeJson(jsonDoc, jsonString);
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

    JsonDocument responsedoc;
    responsedoc["endpoint"] = "/stationcfgupdate";
    responsedoc["updateid"] = "stationupdate";
    responsedoc["updatemsg"] = "<p class='update'> Connecting to WiFi Network </p>";
    responsedoc["timeout"] = "1000";
    String jsonString;
    serializeJson(responsedoc, jsonString);

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

    WiFi.begin(_sta_ssid.c_str(), _sta_pass.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Connecting to WiFi...");
        _sta_attempts++;
        if (_sta_attempts > 10)
        {
            ESP_LOGE(TAG, "Failed to connect to WiFi");
            break;
        }
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
            responsedoc["updateid"] = "stationupdate";
            responsedoc["updatemsg"] = "<p class='updatebad'> Failed To Connect To WiFi! Credentials not saved </p>";
        }
        else
        {
            ESP_LOGI(TAG, "Failed to connect to WiFi, retrying");
            responsedoc["endpoint"] = "/stationcfgupdate";
            responsedoc["updateid"] = "stationupdate";
            responsedoc["updatemsg"] = "<p class='update'> Attempting to connect ... </p>";
            responsedoc["timeout"] = "1000";
        }
    }
    else
    {
        responsedoc["updateid"] = "stationupdate";
        responsedoc["updatemsg"] = "<p class='updategood'> Connected to WiFi network!  <br> SSID and password are saved!</p>";
        _config_helper->setConfigOption(STATION_SSID_KEY, _sta_ssid.c_str());
        _config_helper->setConfigOption(STATION_PASS_KEY, _sta_pass.c_str());
    }

    String jsonString;
    serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}
