#include "comms_handler.h"
#include "A76XX.h"
#include <Preferences.h>
#include <esp_log.h>
#include <HTTPClient.h>

/* -------------------------------------------------------------------------- */
/*                              WIFI CONFIG PAGE                              */
/* -------------------------------------------------------------------------- */
void CommsHandler::WiFi_config_page_init()
{
    // TODO: Handle connecting to WiFi network too somewhere

    // Setup server things
    _server = new AsyncWebServer(80);
    _dnsServer = new DNSServer();
    _ap_ip = IPAddress(192, 168, 4, 1);
    _net_msk = IPAddress(255, 255, 255, 0);
    createJsonConfig(false);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(_ap_ip, _ap_ip, _net_msk);

    if (strlen(getConfigOptionString("appass", WIFI_PASS_AP).c_str()) < 8)
    {
        WiFi.softAP(getConfigOptionString("apssid", WIFI_SSID_AP).c_str());
    }
    else
    {
        WiFi.softAP(getConfigOptionString("apssid", WIFI_SSID_AP).c_str(),
                    getConfigOptionString("appass", WIFI_PASS_AP).c_str());
    }
    // Wait for a second for AP to boot up
    vTaskDelay(pdMS_TO_TICKS(1000));

    _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer->start(_dns_port, "*", _ap_ip);

    xTaskCreate(WiFi_config_page_task, "WiFi_config_page_task", 4096, this, 1, NULL);

    _server->on("/", [this](AsyncWebServerRequest *request)
                { this->WiFi_config_handle_root(request); });

    _server->on("/generate_204", [this](AsyncWebServerRequest *request)
                { this->WiFi_config_handle_root(request); });

    _server->on("/fwlink", [this](AsyncWebServerRequest *request)
                { this->WiFi_config_handle_root(request); });

    _server->onNotFound([this](AsyncWebServerRequest *request)
                        { this->WiFi_config_handle_root(request); });

    // FUNCTION CALLBACKS
    // WiFi Scanning
    _server->on("/startscan", HTTP_GET, [this](AsyncWebServerRequest *request)
                { this->StationScanCallbackStart(request); });
    _server->on("/getscan", HTTP_GET, [this](AsyncWebServerRequest *request)
                { this->StationScanCallbackReturn(request); });
    _server->on("/stationcfg", [this](AsyncWebServerRequest *request)
                { this->StationSetConfig(request); });
    _server->on("/stationcfgupdate", [this](AsyncWebServerRequest *request)
                { this->StationSendUpdate(request); });

    _server->on("/apcfg", [this](AsyncWebServerRequest *request)
                { this->AccessPointSetConfig(request); });

    _server->on("/cfgfr", [this](AsyncWebServerRequest *request)
                { this->factoryResetConfigCallback(request); });

    _server->on("/cfggetjson", HTTP_GET, [this](AsyncWebServerRequest *request)
                { this->sendConfigJsonCallback(request); });

    // Simple Firmware Update Form
    _server->on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"); });
    _server->on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
                {
                    bool shouldReboot = !Update.hasError();
                    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
                    response->addHeader("Connection", "close");
                    request->send(response); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
                {
                    if(!index){
                    Serial.printf("Update Start: %s\n", filename.c_str());
                    if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
                        Update.printError(Serial);
                    }
                    }
                    if(!Update.hasError()){
                    if(Update.write(data, len) != len){
                        Update.printError(Serial);
                    }
                    }
                    if(final){
                    if(Update.end(true)){
                        Serial.printf("Update Success: %uB\n", index+len);
                    } else {
                        Update.printError(Serial);
                    }
    } });

    _server->begin();
}

void CommsHandler::WiFi_config_page_task(void *pvParameters)
{
    auto *instance = static_cast<CommsHandler *>(pvParameters);
    for (;;)
    {
        instance->_dnsServer->processNextRequest();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



void CommsHandler::WiFi_config_handle_root(AsyncWebServerRequest *request)
{
    //AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML_GZ_PROGMEM, HTML_GZ_PROGMEM_LEN);
    //response->addHeader("Content-Encoding", "gzip");
    //request->send(response);
}



/* -------------------------------------------------------------------------- */
/*                       CONFIGURATION CONFIG FUNCTIONS                       */
/* -------------------------------------------------------------------------- */
void CommsHandler::sendConfigJsonCallback(AsyncWebServerRequest *request)
{
    setDefaultJsonConfig(&_config_json);
    String jsonString;
    serializeJson(_config_json, jsonString);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString);
    request->send(response);
    ESP_LOGI("ConfigHandler", "Sent Config JSON: [%s]", jsonString.c_str());
}

void CommsHandler::handleConfigJsonCallback(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("ConfigHandler", "Callback Received [Params: %d]", params);
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("ConfigHandler", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), "config") == 0)
        {
        }
    }

    JsonDocument responsedoc;
    responsedoc["updateid"] = "configupdate";
    responsedoc["updatemsg"] = "<p class='updategood'> Configuration has been saved! </p>";
    String jsonString;
    serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
    ESP_LOGI("ConfigHandler", "Configuration Updated");
}

void CommsHandler::factoryResetConfigCallback(AsyncWebServerRequest *request)
{
    createJsonConfig(true);
    JsonDocument responsedoc;
    responsedoc["updateid"] = "configfrupdate";
    responsedoc["updatemsg"] = "<p class='updategood'> Configuration has been reset! </p>";
    String jsonString;
    serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
    ESP_LOGI("ConfigHandler", "Factory Reset Config");
}

/* -------------------------------------------------------------------------- */
/*                          STATION CONFIG FUNCTIONS                          */
/* -------------------------------------------------------------------------- */
void CommsHandler::StationScanCallbackStart(AsyncWebServerRequest *request)
{
    ESP_LOGI("WiFi Card", "Callback Received: Scanning for networks");
    // Start Asynchronous scan for WiFi networks
    WiFi.scanNetworks(true);
    // Get the number of networks found
    int num_networks = WiFi.scanComplete();
    // Send the number of networks found
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("YEET");

    request->send(response);
}

void CommsHandler::StationScanCallbackReturn(AsyncWebServerRequest *request)
{
    // Get the number of networks found
    int num_networks = WiFi.scanComplete();
    ESP_LOGI("WiFi Card", "Callback Received: Returning [%d] networks", num_networks);
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

void CommsHandler::StationSetConfig(AsyncWebServerRequest *request)
{
    wifi_config_attempts = 0;
    int params = request->params();
    ESP_LOGI("StationConfig", "Callback Received [Params: %d]", params);
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("StationConfig", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), "stassid") == 0)
        {
            setConfigOptionString("stassid", p->value().c_str());
        }
        else if (strcmp(p->name().c_str(), "stapass") == 0)
        {
            setConfigOptionString("stapass", p->value().c_str());
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

void CommsHandler::StationSendUpdate(AsyncWebServerRequest *request)
{
    wifi_config_attempts++;
    JsonDocument responsedoc;

    String ssid = getConfigOptionString("stationssid", WIFI_SSID_DEF).c_str();
    String pass = getConfigOptionString("stationpass", WIFI_PASS_DEF).c_str();

    ESP_LOGI("StationConfig", "Attempting connection with SSID [%s] and password [%s]", ssid.c_str(), pass.c_str());

    WiFi.begin(ssid.c_str(), pass.c_str());
    if (WiFi.status() != WL_CONNECTED)
    {
        if (wifi_config_attempts > 5)
        {
            ESP_LOGE("StationConfig", "Failed to connect to WiFi");
            responsedoc["updateid"] = "stationupdate";
            responsedoc["updatemsg"] = "<p class='updatebad'> Failed To Connect To WiFi </p>";
        }
        else
        {
            ESP_LOGI("StationConfig", "Failed to connect to WiFi, retrying");
            responsedoc["endpoint"] = "/stationcfgupdate";
            responsedoc["updateid"] = "stationupdate";
            responsedoc["updatemsg"] = "<p class='update'> Attempting to connect ... </p>";
            responsedoc["timeout"] = "1000";
        }
    }
    else
    {
        responsedoc["updateid"] = "stationupdate";
        responsedoc["updatemsg"] = "<p class='updategood'> Connected to WiFi network! </p>";

    }

    String jsonString;
    serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}
/* -------------------------------------------------------------------------- */
/*                        ACCESS POINT CONFIG FUNCTIONS                       */
/* -------------------------------------------------------------------------- */
void CommsHandler::AccessPointSetConfig(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("StationConfig", "Callback Received [Params: %d]", params);
    String apssid = "";
    String appass = "";
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("StationConfig", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), "apssid") == 0)
        {
            apssid = p->value();
        }
        else if (strcmp(p->name().c_str(), "appass") == 0)
        {
            appass = p->value();
        }
    }

    String validationresponse = "";

    if (apssid.length() < 1 || apssid.length() > 32)
    {
        validationresponse += "SSID is not the right length \n";
    }

    // Check if all characters are valid (alphanumeric or allowed special characters)
    for (int i = 0; i < apssid.length(); i++)
    {
        char c = apssid.charAt(i);
        if (!(isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == ' '))
        {
            validationresponse += "SSID has unallowed character(s) \n";
        }
    }

    if ((appass.length() < 8 || appass.length() > 63) && appass.length() != 0)
    {
        validationresponse += "Password is not 0 or between 8 and 64 (inclusive) characters long \n";
    }

    // Check if all characters are valid printable ASCII characters
    for (int i = 0; i < appass.length(); i++)
    {
        char c = appass.charAt(i);
        if (c < 32 || c > 126)
        { // ASCII 32 to 126 are printable characters
            validationresponse += "Password contains unprintable characters \n";
        }
    }
    JsonDocument responsedoc;
    if (validationresponse != "")
    {
        // i.e. validation failed
        responsedoc["updateid"] = "apupdate";
        validationresponse = "<p class='updatebad'>" + validationresponse + "</p>";
        responsedoc["updatemsg"] = validationresponse.c_str();
    }
    else
    {
        responsedoc["updateid"] = "apupdate";
        responsedoc["updatemsg"] = "<p class='updategood'>AP Config seems valid and has been saved!</p>";
        // Save to flash here
        setConfigOptionString("apssid", apssid.c_str());
        setConfigOptionString("appass", appass.c_str());
    }

    String jsonString;
    serializeJson(responsedoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString.c_str());
    request->send(response);
}

/* -------------------------------------------------------------------------- */
/*                            A76XX MODEM FUNCTIONS                           */
/* -------------------------------------------------------------------------- */

static const char *LTE_TAG = "A76XX";
bool CommsHandler::LTE_init()
{
    LTE_modem = new A76XX(SERIAL_LTE);
    LTE_mqtt = new A76XXMQTTClient(*LTE_modem, MQTT_CLIENT_ID, MQTT_USE_SSL);
    SERIAL_LTE.begin(115200, SERIAL_8N1, LTE_PIN_RX, LTE_PIN_TX);
    return true;
}

bool CommsHandler::LTE_connect()
{
    // Startup the modem
    if (LTE_modem->init() == false)
    {
        ESP_LOGE(LTE_TAG, "Modem Initialization failed");
        return false;
    }
    if (LTE_modem->waitForRegistration() == false)
    {
        ESP_LOGE(LTE_TAG, "Registration timed out");
        return false;
    }
    if (LTE_modem->GPRSConnect(LTE_APN) == false)
    {
        ESP_LOGE(LTE_TAG, "GPRS Connection failed");
        return false;
    }

    ESP_LOGI(LTE_TAG, "Connected to network");

    // Connect to MQTT
    if (LTE_mqtt->begin() == false)
    {
        if (LTE_mqtt->getLastError() == A76XX_MQTT_ALREADY_STARTED)
        {
            ESP_LOGE(LTE_TAG, "MQTT service already started");
        }
        else
        {
            ESP_LOGE(LTE_TAG, "MQTT service failed to start: %d", LTE_mqtt->getLastError());
            return false;
        }
    }

    if (LTE_mqtt->connect(MQTT_SERVER, MQTT_PORT, MQTT_CLEAN_SESSION, MQTT_KEEPALIVE, NULL, NULL, MQTT_LWT_TOPIC, MQTT_LWT_MESSAGE, MQTT_LWT_QOS) == false)
    {
        ESP_LOGE(LTE_TAG, "MQTT connection failed [%d]", LTE_mqtt->getLastError());
        // return false;
    }

    ESP_LOGI(LTE_TAG, "Connected to MQTT server");

    return true;
}

bool CommsHandler::LTE_pub(const char *topic, const char *message, int qos)
{
    return LTE_mqtt->publish(topic, message, qos, LTE_PUBLISH_TIMEOUT_S);
}

bool CommsHandler::LTE_disable()
{
    return false;
}

/* -------------------------------------------------------------------------- */
/*                               WIFI FUNCTIONS                               */
/* -------------------------------------------------------------------------- */

static const char *WIFI_TAG = "WiFi";

bool CommsHandler::WiFi_init()
{
    WiFi_client = new WiFiClient();
    WiFi_mqtt = new MQTTClient(1024);
    return true;
}

bool CommsHandler::WiFi_connect()
{
    int num_attempts = 0;

    if (WiFi.status() != WL_CONNECTED)
    {
        ESP_LOGE(WIFI_TAG, "Not connected to WiFi, running configurator");
        return false;
    }

    WiFi_mqtt->begin(MQTT_SERVER, MQTT_PORT, *WiFi_client);
    num_attempts = 0;
    while (!WiFi_mqtt->connect(MQTT_CLIENT_ID))
    {
        ESP_LOGE(WIFI_TAG, "Waiting to connect to MQTT Server ...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        num_attempts++;
        if (num_attempts > 5)
        {
            ESP_LOGE(WIFI_TAG, "Failed to connect to MQTT Server");
            return false;
        }
    }
    return true;
}

bool CommsHandler::WiFi_connected()
{
    return (WiFi.status() == WL_CONNECTED);
}

bool CommsHandler::WiFi_pub(const char *topic, const char *message, int qos)
{
    return WiFi_mqtt->publish(topic, message, strlen(message), MQTT_RETAIN, qos);
}

bool CommsHandler::WiFi_disable()
{
    return false;
}

/* -------------------------------------------------------------------------- */
/*                           OVERALL MQTT FUNCTIONS                           */
/* -------------------------------------------------------------------------- */

const char *O_TAG = "CommsHandler";

void CommsHandler::MQTT_queue_pub(const char *topic, const char *message, int qos)
{
    MQTT_pub_t pub;
    pub.topic = new char[strlen(topic) + 1];
    pub.message = new char[strlen(message) + 1];
    strcpy(pub.topic, topic);
    strcpy(pub.message, message);
    pub.qos = qos;

    xQueueSend(MQTT_pub_queue, &pub, 0);
}

bool CommsHandler::MQTT_sub(const char *topic)
{
    return false;
}

void CommsHandler::MQTT_manage_task(void *pvParameters)
{
    auto *instance = static_cast<CommsHandler *>(pvParameters);
    bool pub_success = true;

    bool use_wifi = WIFI_PRIORITIZED;
    bool on_secondary = false;
    TickType_t last_switch_ticks = xTaskGetTickCount();
    MQTT_pub_t pub;
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (instance->Comm_state == Comm_Off)
        {
            ESP_LOGI(O_TAG, "Initializing Connection");
            if (use_wifi == true)
            {
                if (instance->WiFi_init() == false)
                {
                    ESP_LOGE(O_TAG, "WiFi Initialization failed");
                    instance->Comm_state = Comm_Fail;
                    continue;
                }
                if (instance->WiFi_connect() == false)
                {
                    ESP_LOGE(O_TAG, "WiFi Connection failed");
                    instance->Comm_state = Comm_Fail;
                    continue;
                }
                instance->Comm_state = Comm_WiFi;
            }
            else
            {
                if (instance->LTE_init() == false)
                {
                    ESP_LOGE(O_TAG, "LTE Initialization failed");
                    instance->Comm_state = Comm_Fail;
                    continue;
                }
                if (instance->LTE_connect() == false)
                {
                    ESP_LOGE(O_TAG, "LTE Connection failed");
                    instance->Comm_state = Comm_Fail;
                    continue;
                }
                instance->Comm_state = Comm_LTE;
            }
            continue;
        }
        else if (instance->Comm_state == Comm_LTE)
        {
            if (xQueueReceive(instance->MQTT_pub_queue, &pub, pdMS_TO_TICKS(300)))
            {
                pub_success = instance->LTE_pub(pub.topic, pub.message, pub.qos);
                if (pub_success == false)
                {
                    ESP_LOGE(O_TAG, "Failed to publish message via LTE");
                    instance->Comm_state = Comm_Fail;
                    // Re add to queue to avoid shit being deleted
                    xQueueSendToFront(instance->MQTT_pub_queue, &pub, pdMS_TO_TICKS(100));
                }
                else
                {
                    ESP_LOGI(O_TAG, "Published message via LTE");
                    delete pub.topic;
                    delete pub.message;
                }
            }
        }
        else if (instance->Comm_state == Comm_WiFi)
        {
            if (xQueueReceive(instance->MQTT_pub_queue, &pub, pdMS_TO_TICKS(300)))
            {
                pub_success = instance->WiFi_pub(pub.topic, pub.message, pub.qos);
                if (pub_success == false)
                {
                    ESP_LOGE(O_TAG, "Failed to publish message via WiFi");
                    instance->Comm_state = Comm_Fail;
                    // Send to front to maintain order and stop things being lost
                    xQueueSendToFront(instance->MQTT_pub_queue, &pub, pdMS_TO_TICKS(100));
                }
                else
                {
                    ESP_LOGI(O_TAG, "Published message via WiFi");
                    delete pub.topic;
                    delete pub.message;
                }
            }
        }
        else if (instance->Comm_state == Comm_Fail)
        {
            ESP_LOGE(O_TAG, "Switching comms method");
            // Handle switching logic here
            // if use_wifi is true, then last connection attempt was wifi
            // if use_wifi is false, then last connection attempt was LTE
            use_wifi = !use_wifi;
            on_secondary = !on_secondary;
            last_switch_ticks = xTaskGetTickCount();
            instance->Comm_state = Comm_Off;
            continue;
        }
        else
        {
            ESP_LOGE(O_TAG, "Unknown Comm State");
        }

        // Switch back to WiFi if connected to WiFi and it is prioritized and timeout reached
        if (WIFI_PRIORITIZED && use_wifi == false)
        {
            if (xTaskGetTickCount() - last_switch_ticks > pdMS_TO_TICKS(PRIMARY_CHECK_TIME_MS))
            {
                use_wifi = true;
                instance->Comm_state = Comm_Off;
            }
        }
    }
}

void CommsHandler::MQTT_init()
{
    MQTT_pub_queue = xQueueCreate(10, sizeof(MQTT_pub_t));
    WiFi_state = WiFi_Off;
    LTE_state = LTE_Disabled;
    Comm_state = Comm_Off;
    xTaskCreatePinnedToCore(MQTT_manage_task, "MQTT_manage_task", 4096, this, 1, NULL, 1);
    return void();
}
