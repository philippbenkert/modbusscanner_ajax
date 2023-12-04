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
#include "OTAUpdates.h"
#include "ModbusSettings.h"

// Konstanten
//#define BOARD_POWER_ON        4
#define BOARD_485_EN            2
//#define GPIO_PUSE             16

// Globale Objekte
RTC_DS3231 rtc;
LGFX display;
SDCardHandler sdCard;
WebServer webServer;
OTAUpdates otaUpdates;
extern ModbusScanner modbusScanner;
extern DNSServer dnsServer;
extern void connectToWifi(const char* ssid, const char* password, lv_obj_t* popup);
extern bool loadSTACredentials(String &ssid, String &password);
extern void updateShouldReconnect(bool connect);
extern void updateProgressChart(lv_obj_t* chart, const Recipe& recipe, unsigned long currentTime);
            


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

void setup() {
    isConnectedModbus = false;
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
    loadCoolingProcessStatus();
    
    sdCard.init();
    webServer.begin();
    modbusScanner.begin();
    display.init(); 
    display.lvgl_init();
    otaUpdates.begin("savedSSID", "savedPassword");

    String ssid, password;
    if (loadSTACredentials(lastSSID, lastPassword)) {
        connectToWifi(lastSSID.c_str(), lastPassword.c_str(), popup);
    }
    modbusScanner.begin();
    //isConnectedModbus = modbusScanner.isClientReachable(); // Überprüfe, ob der Modbus-Client erreichbar ist

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
    }
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

if (chart && lv_obj_is_valid(chart)) {
    static unsigned long lastUpdateTime = 0; // Speichert den Zeitpunkt der letzten Aktualisierung
    const unsigned long updateInterval = 10000; // 5 Minuten in Millisekunden

    unsigned long currentTime1 = millis(); // Aktuelle Zeit in Millisekunden seit dem Start des Programms

    if (currentTime1 - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime1; // Aktualisieren des Zeitpunkts der letzten Aktualisierung
    void updateRecipeDropdownState();
    void updateSaveButtonState();
    if (coolingProcessRunning) {
        DateTime currentTime = rtc.now(); // Aktuelle Zeit vom RTC
        //createProgressChart(chart, getCurrentRecipe());
        unsigned long currentUnixTime = currentTime.unixtime(); // Extrahiert den Unix-Zeitstempel
        updateProgressChart(chart, getCurrentRecipe(), currentUnixTime);    
        isMenuLocked = true;
    } else {
        isMenuLocked = false;
    }
    }
        bool cursorVisible = !coolingProcessRunning;
        updateCursorVisibility(chart, cursorVisible);

        lv_color_t seriesColor = coolingProcessRunning ? lv_color_make(192, 192, 192) : lv_palette_main(LV_PALETTE_GREEN);
        updateSeriesColor(chart, seriesColor);

        lv_chart_refresh(chart); // Aktualisieren Sie das Chart, um die Änderungen anzuzeigen
    }
}
