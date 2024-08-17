#include "comms_handler.h"
#include "A76XX.h"
#include <esp_log.h>

// TODO: handle reset via button press
// TODO: multi wifi

CommsHandler::CommsHandler()
{
    // Initial states
    _state = Comms_Unititialized;
    _error = Comms_No_Error;

    // Instantiate sub components
    _config_helper = new ConfigHelper(false);
    _ota_helper = new OTAHelper(_config_helper);
    // TODO: address the station and wifi initialization
    _config_webpage = new ConfigWebpage(_config_helper, _ota_helper);

    // Load current configuration
    _load_current_config();

    // Instantiate a76xx client
    SERIAL_LTE.begin(115200, SERIAL_8N1, A76XX_PIN_RX, A76XX_PIN_TX);
    _a76xx = new A76XX(SERIAL_LTE);

    _mqtt_msg_queue = xQueueCreate(_size_mqtt_msg_queue, sizeof(MQTTMessage_t));
    _led_blink_pattern = xQueueCreate(1, sizeof(BlinkPattern_t));

    // Start the tasks
    xTaskCreate(CommsHandlerManagementTask, "CommsHandlerManagementTask", 4096, this, 1, NULL);
    // xTaskCreate(CommsHandlerQueueTask, "CommsHandlerQueueTask", 4096, this, 1, NULL);

    // Start the LED pattern task
    xTaskCreate(IndicatorLEDPatternTask, "IndicatorLEDPatternTask", 4096, this, 1, NULL);
    setIndicatorLEDPattern(3, 1000, 1000, 1000);
}

void CommsHandler::_load_current_config()
{
    // TODO: implement basic error handling
    _mqtt_client_id = _config_helper->getConfigOption(MQTT_CLIENT_ID_KEY, "");
    _mqtt_server = _config_helper->getConfigOption(MQTT_SERVER_URL_KEY, "");
    _mqtt_port = _config_helper->getConfigOption(MQTT_SERVER_URL_KEY, 0);

    _mqtt_username = _config_helper->getConfigOption(MQTT_USERNAME_KEY, "");
    _mqtt_password = _config_helper->getConfigOption(MQTT_PASSWORD_KEY, "");

    _size_mqtt_msg_queue = _config_helper->getConfigOption(MQTT_QUEUE_SIZE_KEY, 1);
}

// Self management task
void CommsHandler::CommsHandlerManagementTask(void *pvParameters)
{
    CommsHandler *comms_handler = (CommsHandler *)pvParameters;
    for (;;)
    {
        // Check if there is a message in the queue (short check time)

        // If none check and update current state

        // LED TEST
        comms_handler->setIndicatorLEDPattern(3, 1000, 1000, 1000);
        vTaskDelay(pdMS_TO_TICKS(5000));
        comms_handler->setIndicatorLEDPattern(5, 100,300,100,100,300);
        vTaskDelay(pdMS_TO_TICKS(5000));
        comms_handler->setIndicatorLEDPattern(0);
        vTaskDelay(pdMS_TO_TICKS(5000));
        comms_handler->setIndicatorLEDPattern(5, 100,100,100,100,100);
        vTaskDelay(pdMS_TO_TICKS(5000));
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
            ESP_LOGI("CommsHandler", "Received message from queue");
            ESP_LOGI("CommsHandler", "Topic: %s", mqtt_message.topic.c_str());
            ESP_LOGI("CommsHandler", "Payload: %s", mqtt_message.payload.c_str());
        }
    }
}

/* ----------------------------- A76XX FUNCTIONS ---------------------------- */
void CommsHandler::_a76xx_power_on()
{
    // Cycle power to GPIO pin here (can be blocking maybe)
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
        if (xQueueReceive(comms_handler->_led_blink_pattern, &pattern, 0))
        {
            ESP_LOGI("CommsHandler", "Received LED pattern");
        }

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