#include "lvgl.h"
#include <WiFi.h>
#include "webserver.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ModbusScanner.h"
#include "WebSocketHandler.h"
#include "TouchPanel.h"
#include "SDCardHandler.h"
#include <LovyanGFX.hpp>

//#define BOARD_POWER_ON              4

#define BOARD_485_EN                2
//#define GPIO_PUSE                   16

LGFX display;
SDCardHandler sdCard;

std::string ssid;
std::string password;

WebServer webServer;
extern ModbusScanner modbusScanner;

bool loadCredentials(String& savedSSID, String& savedPassword) {
    if (LittleFS.exists("/config/wlan-credentials.json")) {  // Ändern Sie den Dateinamen zu wlan-credentials.json
        File file = LittleFS.open("/config/wlan-credentials.json", "r");
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

String getWlanSettingsAsJson() {
    String savedSSID, savedPassword;
    if(loadCredentials(savedSSID, savedPassword)) {
        DynamicJsonDocument doc(1024);
        doc["ssid"] = savedSSID;
        doc["password"] = savedPassword;
        String output;
        serializeJson(doc, output);
        return output;
    }
    return "{}"; // Leeres JSON, wenn keine Einstellungen gefunden wurden
}

void setup() {
    // Serial-Kommunikation beginnen
    //pinMode(GPIO_PUSE, OUTPUT);
    // ... (Ihr anderer Initialisierungscode, falls vorhanden)

    // Peripheral power supply is enabled, the Pin must be set to HIGH to use PCIE, RS485
    //pinMode(BOARD_POWER_ON, OUTPUT);
    //digitalWrite(BOARD_POWER_ON, HIGH);

    pinMode(BOARD_485_EN, OUTPUT);
    digitalWrite(BOARD_485_EN, LOW);

    Serial.begin(115200);

    if (!LittleFS.begin(true)) {
        return;
    }

    String savedSSID, savedPassword;
    if(loadCredentials(savedSSID, savedPassword)) {
        // Access Point (AP) erstellen mit den geladenen Anmeldeinformationen
        if(WiFi.softAP(savedSSID.c_str(), savedPassword.c_str())) {
        } else {
            return;
        }

        IPAddress IP = WiFi.softAPIP();
    } else {
    }

    // Webserver starten
    sdCard.init();
    webServer.begin();
    modbusScanner.begin();
    display.init(); 
    display.lvgl_init();
    // Weitere Setup-Code...
}

void loop() {
    static uint32_t nextTick = 0;
    uint32_t currentTime = millis();

    if(currentTime > nextTick) {
        nextTick = currentTime + lv_timer_handler();
    }

    lv_task_handler();
    delay(5);  // Ein kurzes Delay kann hilfreich sein

}