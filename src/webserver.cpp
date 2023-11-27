#include "WebServer.h"
#include <LittleFS.h>
#include "ModbusScanner.h"
#include <ArduinoJson.h>
#include "WebSocketHandler.h"
#include <DNSServer.h>

constexpr char WLAN_CREDENTIALS_FILE[] = "/config/wlan-credentials.json";
constexpr char MODBUS_CONFIG_FILE[] = "/config/modbus-config.json";
constexpr size_t BUFFER_SIZE = 256;

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer::WebServer() : server(80) {}
AsyncWebSocket ws("/ws"); // Erstellen Sie ein WebSocket-Objekt

bool WebServer::saveToFile(const char *path, const char *data) {
    File file = LittleFS.open(path, "w");
    if (!file || !file.print(data)) return false;
    file.close();
    return true;
}

bool WebServer::readFromFile(const char *path, char *data, size_t size) {
    File file = LittleFS.open(path, "r");
    if (!file) return false;
    size_t bytesRead = file.readBytes(data, size - 1);
    data[bytesRead] = '\0';
    file.close();
    return bytesRead > 0;
}

extern ModbusScanner modbusScanner;

void WebServer::begin() {
    if (!LittleFS.begin() && !LittleFS.format() && !LittleFS.begin()) {
        return;
    }
    // WebSocket-Handler hinzufügen
    wsHandler.bindToServer(&server);
    server.addHandler(&ws);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/manualscan", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasHeader("Content-Type") && request->header("Content-Type").equalsIgnoreCase("application/json")) {
        // Der Body enthält JSON-Daten
        AsyncWebParameter* p = request->getParam(0, true, true); // POST data
        if (p && p->value().length() > 0) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, p->value());
            
            if (doc.containsKey("startregister") && doc.containsKey("length") && doc.containsKey("function")) {
                uint16_t startregister = doc["startregister"];
                uint8_t length = doc["length"];
                uint8_t func = doc["function"];
                
                // Führen Sie hier Ihren Code aus, um den Scan durchzuführen...
                
            } else {
                request->send(400, "text/plain", "Fehlende Parameter für den manuellen Scan");
            }
        } else {
            request->send(400, "text/plain", "Leerer Body");
        }
    } else {
        request->send(400, "text/plain", "Ungültiger Content-Type");
    }
    });

    server.on("/get-modbus-settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        const char* filename = "/config/modbus-config.json";
        if (LittleFS.exists(filename)) {
            char jsonResponse[256]; // Passen Sie die Größe entsprechend an
            if (readFromFile(filename, jsonResponse, sizeof(jsonResponse))) {
                request->send(200, "application/json", jsonResponse);
            } else {
                request->send(500, "text/plain; charset=UTF-8", "Fehler beim Abrufen der Modbus-Einstellungen.");
            }
        } else {
            request->send(404, "text/plain; charset=UTF-8", "Einstellungen nicht gefunden");
        }
    });

    server.on("/set-wlan", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        char newSSID[64]; // Passen Sie die Größe entsprechend an
        char newPwd[64]; // Passen Sie die Größe entsprechend an

        request->getParam("ssid", true)->value().toCharArray(newSSID, sizeof(newSSID));
        request->getParam("password", true)->value().toCharArray(newPwd, sizeof(newPwd));

        char jsonConfig[128]; // Passen Sie die Größe entsprechend an
        snprintf(jsonConfig, sizeof(jsonConfig), "{\"ssid\":\"%s\",\"password\":\"%s\"}", newSSID, newPwd);

        // Speichern Sie die Einstellungen in einer .json-Datei
        if (saveToFile("/config/wlan-credentials.json", jsonConfig)) {
            request->send(200, "application/json", "{\"success\":true, \"message\":\"Einstellungen erfolgreich gespeichert.\"}");
        } else {
            request->send(500, "text/plain; charset=UTF-8", "Fehler beim Speichern der WLAN-Einstellungen.");
        }
    }
    });

    server.on("/get-wlan-settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        const char* filename = "/config/wlan-credentials.json";
        char jsonData[128]; // Passen Sie die Größe entsprechend an
        if (readFromFile(filename, jsonData, sizeof(jsonData))) {
            request->send(200, "application/json", jsonData);
        } else {
            request->send(500, "text/plain; charset=UTF-8", "Fehler beim Abrufen der WLAN-Einstellungen.");
        }
    });

    // Verwenden Sie serveStatic, um das gesamte Dateisystem-Verzeichnis zu servieren
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    const char* routes[] = { "/wlan-settings", "/file-management", "/modbus-settings", "/modbus-scanner" };
    for (const char* route : routes) {
        server.serveStatic(route, LittleFS, route).setDefaultFile((String(route).substring(1) + ".html").c_str());
    }

    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/bootstrap.min.css", "text/css");
    });

    server.on("/delete", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    if (!request->hasArg("path")) {
        request->send(400, "text/plain", "Bad Request: Missing path argument");
        return;
    }
    String filePath = request->arg("path");
    if (LittleFS.exists(filePath.c_str())) {
        if (LittleFS.remove(filePath.c_str())) {
            request->send(200, "text/plain; charset=UTF-8", "File successfully deleted");
        } else {
            request->send(500, "text/plain; charset=UTF-8", "Failed to delete file");
        }
    } else {
        request->send(404, "text/plain; charset=UTF-8", "File not found");
    }
    });
    // Server starten
    server.begin();
}


