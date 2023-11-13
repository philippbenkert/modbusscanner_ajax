#include "OTAUpdates.h"

void OTAUpdates::begin(const char* ssid, const char* password) {
    // Verbinden mit WLAN
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    // OTA-Setup
    ArduinoOTA.begin();
}

void OTAUpdates::handle() {
    ArduinoOTA.handle();
}
