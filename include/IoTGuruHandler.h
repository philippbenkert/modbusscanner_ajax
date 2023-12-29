#include "WLANSettings.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

class IoTHandler {
private:
    WLANSettings& wlanSettings;
    String apiSecureKey;
    String deviceId;

public:
    IoTHandler(WLANSettings& wlanSettings, const String& deviceId, const String& apiSecureKey)
    : wlanSettings(wlanSettings), deviceId(deviceId), apiSecureKey(apiSecureKey) {
        // Initialisierung
    }

    void sendValue(const String& sensorName, float value, bool coolingProcessRunning, const String& startTime, const String& savedEndTime, const String& selectedRecipeIndex) {
    if (wlanSettings.isConnected()) {
        String selectedRecipeIndexStr = String(selectedRecipeIndex);
        HTTPClient http;
        http.begin("http://179.43.154.10:8001/receive");
        http.addHeader("Content-Type", "application/json");
        http.addHeader("API-Key", apiSecureKey);

        StaticJsonDocument<300> doc; // Größe erhöhen, falls nötig
        doc["id"] = deviceId;
        doc["sensor_data"][sensorName] = value;
        doc["coolingProcessRunning"] = coolingProcessRunning;
        doc["startTime"] = startTime;
        doc["savedEndTime"] = savedEndTime;
        doc["selectedRecipeIndex"] = selectedRecipeIndexStr;

        String requestBody;
        serializeJson(doc, requestBody);

        int httpResponseCode = http.POST(requestBody);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println(requestBody);
            Serial.println(response);
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("Keine WLAN-Verbindung, Wert kann nicht gesendet werden.");
    }
}


    // Weitere Methoden...
};
