#include <WiFi.h>
#include "webserver.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

std::string ssid;  // Ihr AP-Netzwerkname
std::string password;  // Ihr AP-Netzwerkschlüssel

WebServer webServer;

bool loadCredentials(String& savedSSID, String& savedPassword) {
    if (LittleFS.exists("/wlan-credentials.json")) {  // Ändern Sie den Dateinamen zu wlan-credentials.json
        File file = LittleFS.open("/wlan-credentials.json", "r");
        if (file) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, file);
            savedSSID = doc["ssid"].as<String>();
            savedPassword = doc["password"].as<String>();
            file.close();
            return true;
        }
    }
    return false;
}

void setup() {
    // Serial-Kommunikation beginnen
    Serial.begin(115200);

    if (!LittleFS.begin(true)) {
        Serial.println("Ein Fehler ist beim Mounten von LITTLEFS aufgetreten.");
        return;
    }

    String savedSSID, savedPassword;
    if(loadCredentials(savedSSID, savedPassword)) {
        // Access Point (AP) erstellen mit den geladenen Anmeldeinformationen
        if(WiFi.softAP(savedSSID.c_str(), savedPassword.c_str())) {
            Serial.println("Access Point erstellt.");
        } else {
            Serial.println("Access Point konnte nicht erstellt werden.");
            return;
        }

        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP-Adresse: ");
        Serial.println(IP);
    } else {
        Serial.println("Keine gespeicherten WLAN-Anmeldeinformationen gefunden.");
    }

    // Webserver starten
    webServer.begin();
}


void loop() {
    // Hier kommt später der Modbus-Code und sonstiger periodischer Code
    webServer.handleClient();
}