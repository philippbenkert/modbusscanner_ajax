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
#include "FileManagement.h"
#include "WLANSettings.h"
#include <Wire.h>
#include "RTCControl.h"
#include "DateTimeHandler.h"

// Konstanten
//#define BOARD_POWER_ON              4
#define BOARD_485_EN                2
//#define GPIO_PUSE                   16

// Globale Objekte
RTC_DS3231 rtc;
LGFX display;
SDCardHandler sdCard;
WebServer webServer;
extern ModbusScanner modbusScanner;
extern DNSServer dnsServer;

DateTime getRTCDateTime() {
    return rtc.now();
}

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
        return;
    }

    Wire.begin(10, 11);  // SDA auf GPIO10, SCL auf GPIO11
    if (!rtc.begin()) {
        Serial.println("RTC konnte nicht gefunden werden!");
        while (1);
    }

    if (rtc.lostPower()) {
        Serial.println("RTC hat die Zeit verloren. Setze auf die aktuelle Zeit.");
        // Setze die RTC-Zeit auf die Kompilierungszeit
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Load WLAN credentials
    String savedSSID, savedPassword;
    if(loadCredentials(savedSSID, savedPassword)) {
        // Create Access Point (AP) with loaded credentials
        if(WiFi.softAP(savedSSID.c_str(), savedPassword.c_str())) {
            IPAddress IP = WiFi.softAPIP();
        } else {
            return;
        }
    } else {
    }

    // Initialize other components
    pinMode(BOARD_485_EN, OUTPUT);
    digitalWrite(BOARD_485_EN, LOW);
    loadDSTEnabled(); // Lade den DST-Zustand
    updateDSTStatus(); // Aktualisiere den Systemstatus entsprechend
    readRecipesFromFile();

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

    // RTC-Update (z.B. jede Sekunde)
    static uint32_t lastTime = 0;
    if (millis() - lastTime > 1000) {
        lastTime = millis();
        DateTime now = rtc.now();
        
        // Hier können Sie die aktuelle Zeit und das Datum verwenden
    }
}
