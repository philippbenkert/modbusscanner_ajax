#ifndef OTA_UPDATES_H
#define OTA_UPDATES_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

class OTAUpdates {
public:
    void begin(const char* ssid, const char* password);
    void handle();
};

#endif // OTA_UPDATES_H
