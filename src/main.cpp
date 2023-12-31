#include <WiFi.h>
#include "webserver.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "lvgl.h"
#include "ModbusScanner.h"
#include "WLANSettings.h"
#include "WebSocketHandler.h"
#include "TouchPanel.h"
#include "SDCardHandler.h"
#include <LovyanGFX.hpp>
#include <DNSServer.h>
#include "Process.h"
#include <Wire.h>
#include "RTCControl.h"
#include "DateTimeHandler.h"
#include "OTAUpdates.h"
#include "ModbusSettings.h"
#include "CommonDefinitions.h"
#include "UIHandler.h"
#include "IoTGuruHandler.h"

UIHandler uiHandler;
WLANSettings wlanSettings;
IoTHandler* iotHandler;  // Deklariere als Zeiger
String macAddress;  
#define BOARD_485_EN            2

extern int selectedRecipeIndex;
RTC_DS3231 rtc;
DateTime now;
unsigned long lastTime = 0;
unsigned long lastTime1 = 0;
SDCardHandler sdCard;
WebServer webServer;
OTAUpdates otaUpdates;
extern Preferences preferences;
extern ModbusScanner modbusScanner;
extern DNSServer dnsServer;
extern lv_obj_t* recipe_dropdown;
std::vector<TimeTempPair> dbData;
extern lv_color_t seriesColor;
extern int modbusLogTemp;
unsigned long nextSend = 0;
unsigned long lastCheck = 0;
unsigned long nextSend1 = 0;
unsigned long lastCheck1 = 0;
unsigned long checkInterval = 60000;  // Überprüfen Sie alle 60 Sekunden

extern void updateProgress();
bool shouldReconnect = true; // oder false, je nach gewünschter Standardfunktionalität
String lastSSID;
String lastPassword;
DateTime getRTCDateTime() {
    return rtc.now();
}

void wifiTask(void *parameter);
void uiModbusTask(void *parameter);
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

String removeColons(String str) {
    String result;
    for (char c : str) {
        if (c != ':') {
            result += c;
        }
    }
    return result;
}
void setup() {
    String macAddress = WiFi.macAddress();
    macAddress = removeColons(macAddress);

    iotHandler = new IoTHandler(wlanSettings, macAddress, "01bd26576e1adbc3cd343582784d01e2");

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
    if (wlanSettings.loadSTACredentials(lastSSID, lastPassword)) {
        wlanSettings.connectToWifi(lastSSID.c_str(), lastPassword.c_str());
    }
    WiFi.setAutoReconnect(true);

    xTaskCreatePinnedToCore(wifiTask, "WiFi Task", 10000, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(uiModbusTask, "UI Modbus Task", 10000, NULL, 1, NULL, 1);
    
    modbusScanner.begin();
    seriesColor = coolingProcessRunning ? lv_color_make(192, 192, 192) : lv_palette_main(LV_PALETTE_GREEN);
    updateSeriesColor(chart, seriesColor);
    
}

const char* wl_status_to_string(wl_status_t status) {
    switch (status) {
        case WL_NO_SHIELD: return "WL_NO_SHIELD";
        case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
        case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
        case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
        case WL_CONNECTED: return "WL_CONNECTED";
        case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
        case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
        case WL_DISCONNECTED: return "WL_DISCONNECTED";
        default: return "UNKNOWN";
    }
}

void wifiTask(void *parameter) {
    for (;;) {

        if (millis() - lastTime1 > 1000) {
            lastTime1 = millis();
            now = rtc.now(); // Aktualisiere die globale Variable `now`
            if (nextSend1 < millis()) {
                nextSend1 += 300000; // Nächster Sendezeitpunkt in 300 Sekunden
        dnsServer.processNextRequest();  // DNS-Server aktualisieren
        otaUpdates.handle();
        unsigned long now = millis();
        // Überprüfen Sie den WLAN-Status in regelmäßigen Abständen
        if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WLAN-Verbindung verloren.");
        // Versuchen Sie, die Verbindung wiederherzustellen
        wlanSettings.connectToWifi(lastSSID.c_str(), lastPassword.c_str());
    } else {
        // Führen Sie eine HTTP-Anfrage an eine zuverlässige Website durch, um die Internetverbindung zu überprüfen
        HTTPClient http;
        http.begin("http://www.google.com"); // oder eine andere zuverlässige URL
        int httpCode = http.GET();
        
        if (httpCode > 0) {
            // Erfolgreiche Verbindung zum Internet
            Serial.println("Verbunden mit dem Internet.");
        } else {
            // Verbindung zum Internet fehlgeschlagen
            Serial.println("Keine Verbindung zum Internet.");
            // Sie können hier Maßnahmen ergreifen, um die Verbindung wiederherzustellen
        }

        http.end(); //Schließen Sie die Verbindung
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
        delay(5000); // Verzögerung zur CPU-Entlastung
        Serial.print("WLAN-Status: ");
        Serial.println(wl_status_to_string(WiFi.status()));
    }}
    }}

    void uiModbusTask(void *parameter) {
    for (;;) {

        lv_task_handler();
        display.checkStandby();
        if (millis() - lastTime > 1000) {
            lastTime = millis();
            now = rtc.now(); // Aktualisiere die globale Variable `now`
            if (nextSend < millis()) {
                nextSend += 300000; // Nächster Sendezeitpunkt in 300 Sekunden

                float measuredValue; // Verwende den Messwert von modbusLogTemp

// Debug-Ausgabe des ursprünglichen Wertes
Serial.print("modbusLogTemp Wert: ");
Serial.println(modbusLogTemp);

if (modbusLogTemp == 0.00) {  // Ersetzen Sie 0.0 durch den Wert, der "leer" repräsentiert
    measuredValue = 99.99;
    Serial.println("modbusLogTemp ist 0.00, setze measuredValue auf 999.99");
} else {
    measuredValue = modbusLogTemp;
    Serial.print("Setze measuredValue auf: ");
    Serial.println(measuredValue);
}

// Überprüfen, ob iotHandler initialisiert wurde, bevor du es verwendest
if (iotHandler) {
    Serial.print("Sende Temperaturwert: ");
    Serial.println(measuredValue);
    String startTimeStr = String(startTime);
    String selectedRecipeIndexStr = String (selectedRecipeIndex);
    String savedEndTimeStr = String(savedEndTime);


    iotHandler->sendValue("temperatur",measuredValue ,coolingProcessRunning, startTimeStr, savedEndTimeStr, selectedRecipeIndexStr);
} else {
    Serial.println("iotHandler ist nicht initialisiert!");
}
            }

            std::string buttonName = "connectButton";
            lv_obj_t* button = UIHandler::getButtonByName(buttonName);
            //if (uiHandler.isObjectValid(button)) {
            //    wlanSettings.updateConnectionButton(button);
            //} else {
            //    Serial.println("Button is invalid or not found.");
            //}
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

    
void loop() {
}