#include <Arduino.h>
#include <comms_handler.h>

CommsHandler comms;
unsigned long previousMillis;

void setup()
{
    Serial.setDebugOutput(true);
}


void loop()
{
    comms.MQTT_init();
    ESP_LOGI("Main", "MQTT initialized");
    int message_count = 0;
    char message[50];
    TickType_t MainLastWakeTime = xTaskGetTickCount();
    while (true)
    {
        vTaskDelayUntil(&MainLastWakeTime, pdMS_TO_TICKS(5000));
        sprintf(message, "Message: %d", message_count);
        comms.MQTT_queue_pub("sltdl-test", message, 2);
        ESP_LOGI("Main", "Sending message to queue");
        message_count++;
    }
}