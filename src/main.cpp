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
#include <DNSServer.h>



// Konstanten
//#define BOARD_POWER_ON              4
#define BOARD_485_EN                2
//#define GPIO_PUSE                   16

// Globale Objekte
LGFX display;
SDCardHandler sdCard;
WebServer webServer;
extern ModbusScanner modbusScanner;
extern DNSServer dnsServer;

bool loadCredentials(String& savedSSID, String& savedPassword) {
    if (LittleFS.exists("/config/wlan-credentials.json")) {
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
    return "{}"; // Return empty JSON if no settings were found
}

void setup() {
    // Initialize Serial communication
    Serial.begin(115200);

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("Error initializing LittleFS");
        return;
    }

    // Load WLAN credentials
    String savedSSID, savedPassword;
    if(loadCredentials(savedSSID, savedPassword)) {
        // Create Access Point (AP) with loaded credentials
        if(WiFi.softAP(savedSSID.c_str(), savedPassword.c_str())) {
            IPAddress IP = WiFi.softAPIP();
            Serial.println("AP started with IP: " + IP.toString());
        } else {
            Serial.println("Error starting AP");
            return;
        }
    } else {
        Serial.println("No saved WLAN credentials found");
    }

    // Initialize other components
    pinMode(BOARD_485_EN, OUTPUT);
    digitalWrite(BOARD_485_EN, LOW);
    sdCard.init();
    webServer.begin();
    modbusScanner.begin();
    display.init(); 
    display.lvgl_init();
}

void loop() {
    static uint32_t nextTick = 0;
    uint32_t currentTime = millis();

    if(currentTime > nextTick) {
        nextTick = currentTime + lv_timer_handler();
    }

    lv_task_handler();
    delay(5);  // A short delay can be beneficial
    dnsServer.processNextRequest();  // DNS-Server aktualisieren
    checkStandby();
}
