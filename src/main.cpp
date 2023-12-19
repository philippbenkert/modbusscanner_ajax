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
#include "Process.h"
#include "WLANSettings.h"
#include <Wire.h>
#include "RTCControl.h"
#include "DateTimeHandler.h"
#include "OTAUpdates.h"
#include "ModbusSettings.h"
#include "CommonDefinitions.h"

#define BOARD_485_EN            2

RTC_DS3231 rtc;
DateTime now;
unsigned long lastTime = 0;
SDCardHandler sdCard;
WebServer webServer;
OTAUpdates otaUpdates;
extern Preferences preferences;
extern ModbusScanner modbusScanner;
extern DNSServer dnsServer;
extern lv_obj_t* recipe_dropdown;
std::vector<TimeTempPair> dbData;
extern lv_color_t seriesColor;

extern void connectToWifi(const char* ssid, const char* password, lv_obj_t* popup);
extern bool loadSTACredentials(String &ssid, String &password);
extern void updateShouldReconnect(bool connect);
extern void updateProgress();
bool shouldReconnect = true; // oder false, je nach gewünschter Standardfunktionalität
String lastSSID;
String lastPassword;
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

void wifiTask(void *parameter) {
    for (;;) {

        dnsServer.processNextRequest();  // DNS-Server aktualisieren
        otaUpdates.handle();
        if (!WiFi.isConnected() && shouldReconnect) {
            connectToWifi(lastSSID.c_str(), lastPassword.c_str(), popup);
        }
        if (isConnectedModbus) {
        auto configs = readModbusConfigs("/config/device.json");
        if (modbusScanner.tryConnectAndIdentify(configs)) {
            Serial.println("Gerät identifiziert: " + device);
        } else {
            Serial.println("Keine Geräte identifiziert.");
            isConnectedModbus = false;
            //updateToggleButtonLabel(btn);
        }
        }
        delay(1000); // Verzögerung zur CPU-Entlastung
    }
    }

    void uiModbusTask(void *parameter) {
    for (;;) {

        lv_task_handler();
        display.checkStandby();
        if (millis() - lastTime > 1000) {
        lastTime = millis();
        now = rtc.now(); // Aktualisiere die globale Variable `now`
    }

        if (chart && lv_obj_is_valid(chart)) {
            static unsigned long lastUpdateTime = 0; // Speichert den Zeitpunkt der letzten Aktualisierung
            const unsigned long updateInterval = 300000; // 5 Minuten in Millisekunden
            unsigned long currentTime1 = millis(); // Aktuelle Zeit in Millisekunden seit dem Start des Programms
            updateCursorVisibility(chart, !coolingProcessRunning);
        if (currentTime1 - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime1; // Aktualisieren des Zeitpunkts der letzten Aktualisierung
        void updateRecipeDropdownState();
            if (coolingProcessRunning) {
            updateProgress();
            isMenuLocked = true;
            } 
        }
        if (coolingProcessRunning) {
            isMenuLocked = true;
        } else {
            isMenuLocked = false;
        }       
        }
    }
}
void setup() {
    isConnectedModbus = false;
    Serial.begin(115200);
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
    pinMode(BOARD_485_EN, OUTPUT);
    digitalWrite(BOARD_485_EN, LOW);
    loadDSTEnabled(); // Lade den DST-Zustand
    updateDSTStatus(); // Aktualisiere den Systemstatus entsprechend
    readRecipesFromFile();
    loadCoolingProcessStatus();
    
    sdCard.init();

    // Öffnen der Datenbank
    if (!sdCard.openDatabase("/sd/setpoint.db")) {
    Serial.println("Fehler beim Öffnen der Datenbank.");
    // Behandeln Sie den Fehler
    }
    
    webServer.begin();
    modbusScanner.begin();
    display.init(); 
    display.lvgl_init();
    otaUpdates.begin("savedSSID", "savedPassword");

    if (coolingProcessRunning) {
        if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
            lv_obj_add_flag(recipe_dropdown, LV_OBJ_FLAG_HIDDEN);
        }
        seriesColor = coolingProcessRunning ? lv_color_make(192, 192, 192) : lv_palette_main(LV_PALETTE_GREEN);
    }

    String ssid, password;
    if (loadSTACredentials(lastSSID, lastPassword)) {
        connectToWifi(lastSSID.c_str(), lastPassword.c_str(), popup);
    }
    xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 10000, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(uiModbusTask, "UI Modbus Task", 10000, NULL, 1, NULL, 1);
    
    modbusScanner.begin();
    seriesColor = coolingProcessRunning ? lv_color_make(192, 192, 192) : lv_palette_main(LV_PALETTE_GREEN);
    updateSeriesColor(chart, seriesColor);
    
}
    
void loop() {
}