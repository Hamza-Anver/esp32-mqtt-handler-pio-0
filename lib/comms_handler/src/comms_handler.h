#ifndef _COMMS_HANDLER
#define _COMMS_HANDLER

#include <Arduino.h>
#include <A76XX.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>
#include <WiFi.h>
#include <MQTT.h>

enum InternetPreference
{
    WiFi_Only,
    LTE_Only,
    WiFi_then_LTE,
    LTE_then_WiFi
};

class CommsHandler
{
public:

private:

};

#endif // _COMMS_HANDLER