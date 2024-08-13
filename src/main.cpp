#include <Arduino.h>
#include <comms_handler.h>
#include "OTAUpdate.h"

CommsHandler comms;
unsigned long previousMillis;

OTAUpdate ota("https://litter.catbox.moe/md08sc.bin");

void setup()
{
    comms.WiFi_config_page_init();
}

void loop()
{

    // comms.MQTT_init();
    // ESP_LOGI("Main", "MQTT initialized");
    // int message_count = 0;
    // char message[50];
    // TickType_t MainLastWakeTime = xTaskGetTickCount();
    // while (true)
    // {
    //     vTaskDelayUntil(&MainLastWakeTime, pdMS_TO_TICKS(5000));
    //     sprintf(message, "Message: %d", message_count);
    //     comms.MQTT_queue_pub("sltdl-test", message, 2);
    //     ESP_LOGI("Main", "Sending message to queue");
    //     message_count++;
    // }

    while (true)
    {
        delay(1000);
        ESP_LOGI("Main", "Free heap size: [%d], Max Free Heap Alloc: [%d]", esp_get_free_heap_size(), ESP.getMaxAllocHeap());
        if (millis() - previousMillis > 10000)
        {
            previousMillis = millis();
            if(WiFi.status() == WL_CONNECTED)
            {
                ESP_LOGI("Main", "WiFi connected. Starting OTA update...");
                ota.performUpdate();
            }
            else
            {
                ESP_LOGE("Main", "No WiFi connection. OTA update aborted.");
                continue;
            }
            
        }
    }
}