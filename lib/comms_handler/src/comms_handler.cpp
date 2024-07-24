#include "comms_handler.h"
#include "esp_log.h"
#include "A76XX.h"

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
    wifi_client = new WiFiClient();
    mqtt_client = new MQTTClient(1024);
    return true;
}

bool CommsHandler::WiFi_connect()
{
    int num_attempts = 0;
    WiFi.begin(WIFI_SSID_DEF, WIFI_PASS_DEF);
    while (WiFi.status() != WL_CONNECTED)
    {
        ESP_LOGI(WIFI_TAG, "Waiting to connect to WiFi ...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        num_attempts++;
        if (num_attempts > 5)
        {
            ESP_LOGE(WIFI_TAG, "Failed to connect to WiFi");
            return false;
        }
    }

    mqtt_client->begin(MQTT_SERVER, MQTT_PORT, *wifi_client);
    num_attempts = 0;
    while (!mqtt_client->connect(MQTT_CLIENT_ID))
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
    return mqtt_client->publish(topic, message, strlen(message), MQTT_RETAIN, qos);
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

            instance->Comm_state = Comm_Off;
            continue;

        }
        else
        {
            ESP_LOGE(O_TAG, "Unknown Comm State");
        }
        // Switch back to WiFi if connected to WiFi and it is prioritized
        if(WIFI_PRIORITIZED && use_wifi == false){
            if(instance->WiFi_connected()){
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
    LTE_state = LTE_Off;
    Comm_state = Comm_Off;
    xTaskCreatePinnedToCore(MQTT_manage_task, "MQTT_manage_task", 4096, this, 1, NULL, 1);
    return void();
}
