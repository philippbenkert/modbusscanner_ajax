#include <WiFi.h>
#include "webserver.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ModbusScanner.h"

#define BOARD_POWER_ON              4
#define BOARD_485_TX                39
#define BOARD_485_RX                38
#define BOARD_485_EN                42
#define GPIO_PUSE                   16

std::string ssid;
std::string password;

WebServer webServer;
extern ModbusScanner modbusScanner;

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
    pinMode(GPIO_PUSE, OUTPUT);
    // ... (Ihr anderer Initialisierungscode, falls vorhanden)

    // Peripheral power supply is enabled, the Pin must be set to HIGH to use PCIE, RS485
    pinMode(BOARD_POWER_ON, OUTPUT);
    digitalWrite(BOARD_POWER_ON, HIGH);

    pinMode(BOARD_485_EN, OUTPUT);
    digitalWrite(BOARD_485_EN, LOW);

    // Konfigurieren Sie Serial2 mit den angegebenen Pins
    RTUutils::prepareHardwareSerial(Serial2);
    Serial2.begin(19200, SERIAL_8N1, BOARD_485_RX, BOARD_485_TX);

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
    modbusScanner.begin();

}


void loop() {
    // Hier kommt später der Modbus-Code und sonstiger periodischer Code
    webServer.handleClient();
}