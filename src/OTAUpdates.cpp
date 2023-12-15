#include "OTAUpdates.h"

void OTAUpdates::begin(const char* ssid, const char* password) {
    ArduinoOTA.begin();
}

void OTAUpdates::handle() {
    ArduinoOTA.handle();
}
