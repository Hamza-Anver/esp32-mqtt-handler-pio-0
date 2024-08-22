#include "comms_handler.h"
#include "A76XX.h"
#include <esp_log.h>

// TODO: handle reset via button press
// TODO: multi wifi

static const char *TAG = "CommsHandler";

CommsHandler::CommsHandler()
{
    // Initial states
    _state = Comms_Unititialized;
    _error = Comms_No_Error;

    // Instantiate config helper
    // TODO: handle resets if necessary
    // e.g. one time reset for new OTA update configs
    _config_helper = new ConfigHelper(false);

    // Load current configuration
    _load_current_config();

    // Instantiate a76xx client
    SERIAL_LTE.begin(115200, SERIAL_8N1, A76XX_PIN_RX, A76XX_PIN_TX);
    _lte_client = new A76XX(SERIAL_LTE);

    _mqtt_msg_queue = xQueueCreate(_size_mqtt_msg_queue, sizeof(MQTTMessage_t));
    _led_blink_pattern = xQueueCreate(1, sizeof(BlinkPattern_t));

    // Start the tasks
    // TODO: pin to core, lower stack size
    //xTaskCreate(CommsHandlerWiFiTask, "CommsHandlerWiFiTask", 4096, this, 1, NULL);
    //xTaskCreate(CommsHandlerLTETask, "CommsHandlerLTETask", 4096, this, 1, NULL);

    xTaskCreate(CommsHandlerManagementTask, "CommsHandlerManagementTask", 4096, this, 1, NULL);
    // xTaskCreate(CommsHandlerQueueTask, "CommsHandlerQueueTask", 4096, this, 1, NULL);

    // Start the LED pattern task
    xTaskCreate(IndicatorLEDPatternTask, "IndicatorLEDPatternTask", 4096, this, 1, NULL);
    setIndicatorLEDPattern(3, 1000, 1000, 1000);

    // Instantiate sub modules

    _ota_helper = new OTAHelper(_lte_client);
    // TODO: address the station and wifi initialization
    _config_webpage = new ConfigWebpage(_config_helper, _ota_helper);
}

void CommsHandler::_load_current_config()
{
    // TODO: implement basic error handling
    _mqtt_client_id = _config_helper->getConfigOption(MQTT_CLIENT_ID_KEY, "");
    _mqtt_server = _config_helper->getConfigOption(MQTT_SERVER_URL_KEY, "");
    _mqtt_port = _config_helper->getConfigOption(MQTT_SERVER_PORT_KEY, 0);

    _mqtt_username = _config_helper->getConfigOption(MQTT_USERNAME_KEY, "");
    _mqtt_password = _config_helper->getConfigOption(MQTT_PASSWORD_KEY, "");

    _mqtt_keepalive = _config_helper->getConfigOption(MQTT_KEEPALIVE_KEY, 60);
    _mqtt_clean_session = _config_helper->getConfigOption(MQTT_CLEAN_SESSION_KEY, true);

    _mqtt_lwt_topic = _config_helper->getConfigOption(MQTT_LWT_TOPIC_KEY, "");
    _mqtt_lwt_payload = _config_helper->getConfigOption(MQTT_LWT_PAYLOAD_KEY, "");
    _mqtt_lwt_qos = _config_helper->getConfigOption(MQTT_LWT_QOS_KEY, 0);

    _size_mqtt_msg_queue = _config_helper->getConfigOption(MQTT_QUEUE_SIZE_KEY, 1);

    String net_pref = _config_helper->getConfigOption(NET_PRIORITY_PREF_KEY, "");
    if (net_pref == NET_WIFI_ONLY_PREF_OPTION)
    {
        _net_preference = Comms_WiFi_Only;
    }
    else if (net_pref == NET_LTE_ONLY_PREF_OPTION)
    {
        _net_preference = Comms_LTE_Only;
    }
    else if (net_pref == NET_WIFI_THEN_LTE_PREF_OPTION)
    {
        _net_preference = Comms_WiFi_over_LTE;
    }
    else if (net_pref == NET_LTE_THEN_WIFI_PREF_OPTION)
    {
        _net_preference = Comms_LTE_over_WiFi;
    }
    else
    {
        // Default to wifi over lte
        _net_preference = Comms_WiFi_over_LTE;
    }

    _net_pref_switch_timeout = _config_helper->getConfigOption(NET_PRIORITY_SWITCH_TIME_KEY, 0);

    _lte_apn = _config_helper->getConfigOption(A76XX_APN_NAME_KEY, "mobitel");

    _sta_ssid = _config_helper->getConfigOption(STATION_SSID_KEY, "");
    _sta_pass = _config_helper->getConfigOption(STATION_PASS_KEY, "");

    ESP_LOGI(TAG, "Loaded configuration");
}

// Self management task
void CommsHandler::CommsHandlerManagementTask(void *pvParameters)
{
    CommsHandler *comms_handler = (CommsHandler *)pvParameters;

    int pattern_count = 0;
    for (;;)
    {
        // Check if there is a message in the queue (short check time)

        // If state is uninitialized, initialize the module
        if (comms_handler->_state == Comms_Unititialized)
        {
            if (comms_handler->_net_preference == Comms_WiFi_Only ||
                comms_handler->_net_preference == Comms_WiFi_over_LTE)
            {
                //comms_handler->_wifi_power_on();
            }
            else if (comms_handler->_net_preference == Comms_LTE_Only ||
                     comms_handler->_net_preference == Comms_LTE_over_WiFi)
            {
                // comms_handler->_lte_power_on();
            }
        }

        // Set blinkies
        if (comms_handler->_state == Comms_Unititialized)
        {
            // Long pause then fast blinks unintialized
            ESP_LOGI(TAG, "Setting uninitialized LED pattern");
            comms_handler->setIndicatorLEDPattern(4, 1000, 100, 100, 100);
        }
        else if (comms_handler->_state == Comms_WiFi_Connected_Only ||
                 comms_handler->_state == Comms_WiFi_Connected_Primary ||
                 comms_handler->_state == Comms_WiFi_Connected_Secondary)
        {
            // WiFi Pattern
            // off short, on long
            ESP_LOGI(TAG, "Setting WiFi LED pattern");
            comms_handler->setIndicatorLEDPattern(2, 300, 1200);
        }
        else if (comms_handler->_state == Comms_LTE_Connected_Only ||
                 comms_handler->_state == Comms_LTE_Connected_Primary ||
                 comms_handler->_state == Comms_LTE_Connected_Secondary)
        {
            // LTE Pattern
            // Off long, on short
            ESP_LOGI(TAG, "Setting LTE LED pattern");
            comms_handler->setIndicatorLEDPattern(2, 1200, 300);
        }
        else if (comms_handler->_state == Comms_Disconnected)
        {
            // fast blinks for disconnected
            ESP_LOGI(TAG, "Setting disconnected LED pattern");
            comms_handler->setIndicatorLEDPattern(2, 300, 300);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// Queue handler task
void CommsHandler::CommsHandlerQueueTask(void *pvParameters)
{
    CommsHandler *comms_handler = (CommsHandler *)pvParameters;
    CommsHandler_MQTTMessage_t mqtt_message;
    for (;;)
    {
        if (xQueueReceive(comms_handler->_mqtt_msg_queue, &mqtt_message, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Received message from queue");
            ESP_LOGI(TAG, "Topic: %s", mqtt_message.topic.c_str());
            ESP_LOGI(TAG, "Payload: %s", mqtt_message.payload.c_str());
        }
    }
}

/* -------------------------------------------------------------------------- */
/*                             LTE MQTT FUNCTIONS                             */
/* -------------------------------------------------------------------------- */

/*
Handle connection reconnection
handle subscriptions
*/
void CommsHandler::CommsHandlerLTETask(void *pvParameters)
{
    CommsHandler *instance = (CommsHandler *)pvParameters;
    TickType_t last_switch_time = xTaskGetTickCount();
    bool selfpriority = (instance->_net_preference == Comms_LTE_Only || instance->_net_preference == Comms_LTE_over_WiFi);
    bool selfenabled = (instance->_net_preference != Comms_WiFi_Only);
    A76XX *lte_client = instance->_lte_client;
    ESP_LOGI(TAG, "LTE Task Running");
    for (;;)
    {
        if (selfenabled)
        {
            // Only run if enabled
            if (selfpriority)
            {
                ESP_LOGI(TAG, "LTE identified as priority");
                // First time run
                if (instance->_state == Comms_Unititialized)
                {
                    // Initialize LTE
                    instance->_lte_power_on();

                    if (lte_client->init() == false)
                    {
                        ESP_LOGE(TAG, "Failed to initialize LTE err: [%d]", lte_client->getLastError());
                        // instance->_state = Comms_Disconnected;
                    }
                    else
                    {
                        lte_client->reset();
                        if (lte_client->waitForRegistration() == false)
                        {
                            ESP_LOGE(TAG, "Failed to register to LTE network err: [%d]", lte_client->getLastError());
                        }
                        if (lte_client->GPRSConnect(instance->_lte_apn.c_str()) == false)
                        {
                            ESP_LOGE(TAG, "Failed to connect to LTE network err: [%d]", lte_client->getLastError());
                        }

                        if (instance->_lte_mqtt_init() == false)
                        {
                            ESP_LOGE(TAG, "Failed to initialize MQTT client err: [%d]", instance->_lte_mqtt_client->getLastError());
                        }

                        ESP_LOGI(TAG, "Successfully initialized LTE and MQTT client");
                        instance->_state = Comms_LTE_Connected_Only;
                    }

                    // initialize the mqtt client
                    // TODO: handle using SSL
                }
                else if (instance->_state == Comms_LTE_Connected_Only ||
                         instance->_state == Comms_LTE_Connected_Primary ||
                         instance->_state == Comms_LTE_Connected_Secondary)
                {
                    ESP_LOGI(TAG, "LTE Stuff");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        else
        {
            // If not enabled, just wait
            // Can handle changing config without restarting
            ESP_LOGI(TAG, "LTE not enabled");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

void CommsHandler::_lte_power_on()
{
    // Cycle power to GPIO pin here (can be blocking maybe)
    pinMode(A76XX_PWR_PIN, OUTPUT);
    pinMode(A76XX_PWR_PIN, OUTPUT);
    digitalWrite(A76XX_PWR_PIN, HIGH);
    delay(600);
    digitalWrite(A76XX_PWR_PIN, LOW);
    delay(1500);
}

void CommsHandler::_lte_power_off()
{
    _lte_client->powerOff();
}

bool CommsHandler::_lte_mqtt_init()
{
    // Initialize the mqtt client
    // TODO: handle using SSL
    if (_lte_mqtt_client == nullptr)
        _lte_mqtt_client = new A76XXMQTTClient(*_lte_client, _mqtt_client_id.c_str(), false);

    // Start the client
    if (_lte_mqtt_client->begin() == false)
    {

        if (_lte_mqtt_client->getLastError() == -5)
        {
            ESP_LOGE(TAG, "MQTT already started");
        }

        else
        {
            ESP_LOGE(TAG, "Failed to start MQTT client [%d]", _lte_mqtt_client->getLastError());
            return false;
        }
    }

    // Connect to the server
    // String reformatting (not needed)
    String _mqtt_server_tcp = "" + _mqtt_server;
    bool ret = false;
    int conn_attempts = 0;
    while (true)
    {
        if (_mqtt_username.length() == 0 || _mqtt_password.length() == 0)
        {
            if (_mqtt_lwt_topic.length() == 0)
            {
                ESP_LOGI(TAG, "Connecting to MQTT Server,\n\tURL: [%s]\n\tPort: [%d]\n\tClean: [%s]\n\tKeepAlive: [%d]\n\tUsername[%s]\n\tPassword[%s]\n\tLWT Topic[%s]\n\tLWT Payload[%s]\n\tLWTQOS:[%d]",
                         _mqtt_server_tcp.c_str(),
                         _mqtt_port,
                         _mqtt_clean_session ? "true" : "false",
                         _mqtt_keepalive,
                         "NONE", "NONE", "NONE", "NONE", -1);

                ret = _lte_mqtt_client->connect(_mqtt_server_tcp.c_str(),
                                                _mqtt_port,
                                                _mqtt_clean_session,
                                                _mqtt_keepalive);
            }
            else
            {
                ESP_LOGI(TAG, "Connecting to MQTT Server,\nURL: [%s]\nPort: [%d]\nClean: [%s]\nKeepAlive: [%d]\nUsername[%s]\nPassword[%s]\nLWT Topic[%s]\nLWT Payload[%s]\nLWTQOS:[%d]",
                         _mqtt_server_tcp.c_str(),
                         _mqtt_port,
                         _mqtt_clean_session ? "true" : "false",
                         _mqtt_keepalive,
                         "NONE", "NONE",
                         _mqtt_lwt_topic.c_str(),
                         _mqtt_lwt_payload.c_str(),
                         _mqtt_lwt_qos);

                ret = _lte_mqtt_client->connect(_mqtt_server_tcp.c_str(),
                                                _mqtt_port,
                                                _mqtt_clean_session,
                                                _mqtt_keepalive,
                                                NULL, NULL, // Empty user name and password
                                                _mqtt_lwt_topic.c_str(),
                                                _mqtt_lwt_payload.c_str(),
                                                _mqtt_lwt_qos);
            }
        }
        else
        {
            if (_mqtt_lwt_topic.length() == 0)
            {
                ESP_LOGI(TAG, "Connecting to MQTT Server,\nURL: [%s]\nPort: [%d]\nClean: [%s]\nKeepAlive: [%d]\nUsername[%s]\nPassword[%s]\nLWT Topic[%s]\nLWT Payload[%s]\nLWTQOS:[%d]",
                         _mqtt_server_tcp.c_str(),
                         _mqtt_port,
                         _mqtt_clean_session ? "true" : "false",
                         _mqtt_keepalive,
                         _mqtt_username.c_str(),
                         _mqtt_password.c_str(),
                         "NONE", "NONE", -1);

                ret = _lte_mqtt_client->connect(_mqtt_server_tcp.c_str(),
                                                _mqtt_port,
                                                _mqtt_clean_session,
                                                _mqtt_keepalive,
                                                _mqtt_username.c_str(),
                                                _mqtt_password.c_str());
            }
            else
            {
                ESP_LOGI(TAG, "Connecting to MQTT Server,\nURL: [%s]\nPort: [%d]\nClean: [%s]\nKeepAlive: [%d]\nUsername[%s]\nPassword[%s]\nLWT Topic[%s]\nLWT Payload[%s]\nLWTQOS:[%d]",
                         _mqtt_server_tcp.c_str(),
                         _mqtt_port,
                         _mqtt_clean_session ? "true" : "false",
                         _mqtt_keepalive,
                         _mqtt_username.c_str(),
                         _mqtt_password.c_str(),
                         _mqtt_lwt_topic.c_str(),
                         _mqtt_lwt_payload.c_str(),
                         _mqtt_lwt_qos);

                ret = _lte_mqtt_client->connect(_mqtt_server_tcp.c_str(),
                                                _mqtt_port,
                                                _mqtt_clean_session,
                                                _mqtt_keepalive,
                                                _mqtt_username.c_str(),
                                                _mqtt_password.c_str(),
                                                _mqtt_lwt_topic.c_str(),
                                                _mqtt_lwt_payload.c_str(),
                                                _mqtt_lwt_qos);
            }
        }

        if (ret)
        {
            ESP_LOGI(TAG, "Connected to MQTT server via LTE");
            return true;
        }
        else
        {
            if (conn_attempts > 5)
            {
                ESP_LOGE(TAG, "Failed to connect to MQTT server via LTE");
                return false;
            }
            ESP_LOGE(TAG, "Retrying connection to MQTT server via LTE Attempt [%d/5]", conn_attempts);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

bool CommsHandler::_lte_mqtt_pub(String topic,
                                 String payload,
                                 int qos,
                                 bool retain,
                                 bool dup)
{
    return _lte_mqtt_client->publish(topic.c_str(), payload.c_str(), qos, PUB_TIMEOUT_S, retain, dup);
}

/* -------------------------------------------------------------------------- */
/*                             WIFI MQTT FUNCTIONS                            */
/* -------------------------------------------------------------------------- */

void CommsHandler::CommsHandlerWiFiTask(void *pvParameters)
{
    CommsHandler *instance = (CommsHandler *)pvParameters;

    CommsHandler_Net_Pref_t net_pref = instance->_net_preference;
    bool selfpriority = (net_pref == Comms_WiFi_Only || net_pref == Comms_WiFi_over_LTE);
    bool selfenabled = (net_pref != Comms_LTE_Only);
    for (;;)
    {
        if (selfenabled)
        {
            if (selfpriority)
            {
                if (instance->_state == Comms_Unititialized)
                {
                    instance->_wifi_power_on();
                    
                    if (instance->_wifi_mqtt_init() == false)
                    {
                        ESP_LOGE(TAG, "Failed to initialize MQTT client via WiFi");
                    }else{
                        instance->_state = Comms_WiFi_Connected_Only;
                    }   
                }
                else if (instance->_state == Comms_WiFi_Connected_Only ||
                         instance->_state == Comms_WiFi_Connected_Primary ||
                         instance->_state == Comms_WiFi_Connected_Secondary)
                {
                    ESP_LOGI(TAG, "WiFi Stuff");
                }
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
        else
        {
            ESP_LOGI(TAG, "WiFi not enabled");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

bool CommsHandler::_wifi_power_on()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        ESP_LOGI(TAG, "WiFi already connected");
        return true;
    }
    else
    {
        ESP_LOGI(TAG, "WiFi not connected, connecting");
        if (_sta_ssid.length() == 0 || _sta_pass.length() == 0)
        {
            ESP_LOGE(TAG, "Password or SSID is empty for WiFi");
            return false;
        }
        else
        {
            WiFi.begin(_sta_ssid.c_str(), _sta_pass.c_str());
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED)
            {
                delay(1000);
                ESP_LOGI(TAG, "Connecting to WiFi, attempt: [%d/5]", attempts);
                attempts++;
                if (attempts > 5)
                {
                    ESP_LOGE(TAG, "Failed to connect to WiFi");
                    return false;
                }
            }
            ESP_LOGI(TAG, "Connected to WiFi");
            return true;
        }
    }
}

void CommsHandler::_wifi_power_off()
{
    WiFi.disconnect();
}

bool CommsHandler::_wifi_mqtt_init()
{
    ESP_LOGI(TAG, "Initializing MQTT client via WiFi");
    if (WiFi.status() != WL_CONNECTED)
    {
        ESP_LOGE(TAG, "WiFi not connected");
        return false;
    }else{
        ESP_LOGI(TAG, "WiFi connected");
    }

    // Initialize the mqtt client
    if (_wifi_client == nullptr)
        _wifi_client = new WiFiClient();

    // Max size of packet is 1024 bytes
    if (_wifi_mqtt_client == nullptr)
        _wifi_mqtt_client = new MQTTClient(1024);

    // Start the client
    _wifi_mqtt_client->begin(_mqtt_server.c_str(), _mqtt_port, *_wifi_client);
    ESP_LOGI(TAG, "Started MQTT client: \n Server: [%s]\n Port: [%d]", _mqtt_server.c_str(), _mqtt_port);
    int conn_attempts = 0;
    bool ret = false;
    while (true)
    {
        if (_mqtt_username.length() == 0 || _mqtt_password.length() == 0)
        {
            ESP_LOGI(TAG, "Connecting to MQTT Server:\n Client ID: [%s]\n Username: [%s]\n Password: [%s]",
                     _mqtt_client_id.c_str(),
                     "NONE", "NONE");
            ret = _wifi_mqtt_client->connect(_mqtt_client_id.c_str());
        }
        else
        {
            ESP_LOGI(TAG, "Connecting to MQTT Server:\n Client ID: [%s]\n Username: [%s]\n Password: [%s]",
                     _mqtt_client_id.c_str(),
                     _mqtt_username.c_str(),
                     _mqtt_password.c_str());
            ret = _wifi_mqtt_client->connect(_mqtt_client_id.c_str(), _mqtt_username.c_str(), _mqtt_password.c_str());
        }
        if (ret)
        {
            ESP_LOGI(TAG, "Connected to MQTT server via WiFi");
            return true;
        }
        else
        {
            if (conn_attempts > 5)
            {
                ESP_LOGE(TAG, "Failed to connect to MQTT server via WiFi");
                return false;
            }
            ESP_LOGE(TAG, "Retrying connection to MQTT server via WiFi Attempt [%d/5]", conn_attempts);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        conn_attempts++;
    }
}

/* ------------------------------ INDICATOR LED ----------------------------- */
void CommsHandler::setIndicatorLEDPattern(int count,
                                          ...)
{
    va_list args;
    va_start(args, count);

    BlinkPattern_t pattern;
    pattern.num_blinks = count;
    for (int i = 0; i < count; i++)
    {
        int time = va_arg(args, int);
        pattern.times[i] = time;
    }
    va_end(args);

    xQueueSend(_led_blink_pattern, &pattern, portMAX_DELAY);
}

void CommsHandler::IndicatorLEDPatternTask(void *pvParameters)
{
    CommsHandler *comms_handler = (CommsHandler *)pvParameters;
    BlinkPattern_t pattern;
    pattern.num_blinks = 0;
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;)
    {
        // Keep looping till a new pattern is received
        xQueueReceive(comms_handler->_led_blink_pattern, &pattern, 0);

        for (int i = 0; i < pattern.num_blinks; i++)
        {
            // Blink LED
            digitalWrite(LED_BUILTIN, (i % 2) == 0 ? HIGH : LOW);
            vTaskDelay(pattern.times[i] / portTICK_PERIOD_MS);
        }
        if (pattern.num_blinks == 0)
        {
            // Turn off LED
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}