#include "comms_handler.h"
#include "A76XX.h"
#include <Preferences.h>
#include <esp_log.h>
#include "webpage.h"

/* -------------------------------------------------------------------------- */
/*                       CONFIGURATION HELPER FUNCTIONS                       */
/* -------------------------------------------------------------------------- */
String CommsHandler::getConfigOptionString(const char *option, const char *default_value)
{
    _configops.begin(_conf_namespace, true);
    String value = _configops.getString(option, default_value);
    _configops.end();
    ESP_LOGI("ConfigHelper", "Returning [%s] for key [%s]", value.c_str(), option);
    return value;
}

bool CommsHandler::setConfigOptionString(const char *option, const char *value)
{
    _configops.begin(_conf_namespace, false);
    int retval = _configops.putString(option, value);
    _configops.end();
    ESP_LOGI("ConfigHelper", "Putting [%s] for key [%s] retval: [%d]", value, option, retval);
    return (retval != 0);
}

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

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(_ap_ip, _ap_ip, _net_msk);

    if (strlen(getConfigOptionString("appassword", WIFI_SSID_AP).c_str()) < 8)
    {
        WiFi.softAP(getConfigOptionString("apssid", WIFI_SSID_AP).c_str());
    }
    else
    {
        WiFi.softAP(getConfigOptionString("apssid", WIFI_SSID_AP).c_str(),
                    getConfigOptionString("appassword", WIFI_PASS_AP).c_str());
    }
    // Wait for a second for AP to boot up
    vTaskDelay(pdMS_TO_TICKS(1000));

    _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer->start(_dns_port, "*", _ap_ip);

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
    _server->on("/stationcfg",[this](AsyncWebServerRequest *request)
                { this->StationSetConfig(request); });

    xTaskCreate(WiFi_config_page_task, "WiFi_config_page_task", 4096, this, 1, NULL);

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

    request->send_P(200, "text/html", FULL_CONFIG_PAGE);
}

/* ------------------------ STATION CONFIG FUNCTIONS ------------------------ */
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
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print(jsonString);
    request->send(response);
}

void CommsHandler::StationSetConfig(AsyncWebServerRequest *request)
{
    int params = request->params();
    ESP_LOGI("Access Point Card", "Callback Received [Params: %d]", params);
    for (int i = 0; i < params; i++)
    {
        AsyncWebParameter *p = request->getParam(i);
        ESP_LOGI("Access Point Card", "POST[%s]: %s", p->name().c_str(), p->value().c_str());
        if (strcmp(p->name().c_str(), "stationssid") == 0)
        {
            setConfigOptionString("stationssid", p->value().c_str());
        }
        else if (strcmp(p->name().c_str(), "stationpass") == 0)
        {
            setConfigOptionString("stationpass", p->value().c_str());
        }
    }
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    vTaskDelay(pdMS_TO_TICKS(10000));
    response->print("testing2222");
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
