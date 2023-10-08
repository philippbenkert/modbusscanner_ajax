#include "WebServer.h"
#include <LittleFS.h>
#include "ModbusScanner.h"

WebServer::WebServer() : server(80) {}

bool WebServer::isConnectedToModbus() {
    // Platzhalter Logik:
    // Wenn Sie eine spezifische Methode oder Eigenschaft haben, um den Modbus-Verbindungsstatus zu überprüfen,
    // ersetzen Sie den unten stehenden Code durch diese Logik.
    return true; // Gibt immer "verbunden" zurück. Ersetzen Sie dies durch Ihre eigentliche Überprüfungslogik.
}

bool WebServer::saveToFile(const char *path, const char *data) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }
    if (file.print(data)) {
        file.close();
        return true;
    }
    return false;
}

bool WebServer::readFromFile(const char *path, char *data, size_t size) {
    File file = LittleFS.open(path, "r");
    if (!file) return false;

    size_t bytesRead = file.readBytes(data, size - 1); // Lesen Sie size-1 Bytes, um Platz für die Nullterminierung zu lassen
data[bytesRead] = '\0'; // Nullterminierung hinzufügen
    file.close();
    return bytesRead > 0;
}

extern ModbusScanner modbusScanner;

void WebServer::begin() {
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS. Formatting...");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("Failed to mount or format LittleFS");
            return;
        }
    }

    server.on("/list-dir", HTTP_GET, [](AsyncWebServerRequest *request) {
        String output;
        File root = LittleFS.open("/");
        for (File file = root.openNextFile(); file; file = root.openNextFile()) {
            if (output.length()) output += ',';
            output += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
        }
        request->send(200, "application/json", "[" + output + "]");
    });

    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasArg("path")) {
            request->send(400, "text/plain", "Bad Request");
            return;
        }
        char path[256];
        request->arg("path").toCharArray(path, sizeof(path));
        if (LittleFS.exists(path)) {
            request->send(LittleFS, path, String(), true);
        } else {
            request->send(404, "text/plain", "File not found");
        }
    });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "File uploaded");
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File file;
        if (!index) {
            if (LittleFS.exists(filename.c_str())) {
                LittleFS.remove(filename.c_str());
            }
            file = LittleFS.open(filename, "w");
        }
        if (file) {
            file.write(data, len);
            if (final) {
                file.close();
            }
        }
    });

    server.on("/manualscan", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("startregister", true) && request->hasParam("length", true) && request->hasParam("function", true)) {
            uint16_t startregister = request->getParam("startregister", true)->value().toInt();
            uint8_t length = request->getParam("length", true)->value().toInt();
            uint8_t func = request->getParam("function", true)->value().toInt();

            // Scan durchführen
            char result[256]; // Passen Sie die Größe entsprechend an
            snprintf(result, sizeof(result), "%s", modbusScanner.scanFunction(startregister, func).c_str()); // Annahme: Die scanFunction-Methode gibt die gescannten Werte als String zurück
            request->send(200, "text/plain", result);
        } else {
            request->send(400, "text/plain", "Fehlende Parameter für den manuellen Scan");
        }
    });

    server.on("/autoscan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        char result[256]; // Passen Sie die Größe entsprechend an
        snprintf(result, sizeof(result), "%s", modbusScanner.scanRegisters().c_str()); // Annahme: Die scanRegisters-Methode gibt die gescannten Werte als String zurück
        request->send(200, "text/plain", result);
    });

    server.on("/save-modbus-settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("deviceAddress", true) && request->hasParam("baudrate", true) && request->hasParam("parity", true) && request->hasParam("stopbits", true)) {
            char deviceAddress[32]; // Passen Sie die Größe entsprechend an
            char baudrate[32]; // Passen Sie die Größe entsprechend an
            char parity[32]; // Passen Sie die Größe entsprechend an
            char stopbits[32]; // Passen Sie die Größe entsprechend an

            
            request->getParam("deviceAddress", true)->value().toCharArray(deviceAddress, sizeof(deviceAddress));
            request->getParam("baudrate", true)->value().toCharArray(baudrate, sizeof(baudrate));
            request->getParam("parity", true)->value().toCharArray(parity, sizeof(parity));
            request->getParam("stopbits", true)->value().toCharArray(stopbits, sizeof(stopbits));

            char jsonResponse[256]; // Passen Sie die Größe entsprechend an
            snprintf(jsonResponse, sizeof(jsonResponse), "{\"deviceAddress\":\"%s\",\"baudrate\":\"%s\",\"parity\":\"%s\",\"stopbits\":\"%s\"}", deviceAddress, baudrate, parity, stopbits);

            if (saveToFile("/modbus-config.json", jsonResponse)) {
            request->redirect("/modbus-settings.html");
            } else {
            request->send(500, "text/plain", "Fehlende Modbus-Einstellungen");
            }
}});

    server.on("/get-modbus-settings", HTTP_GET, [this](AsyncWebServerRequest *request){
        const char* filename = "/modbus-config.json";
        if (LittleFS.exists(filename)) {
            char jsonResponse[256]; // Passen Sie die Größe entsprechend an
            if (readFromFile(filename, jsonResponse, sizeof(jsonResponse))) {
                request->send(200, "application/json", jsonResponse);
            } else {
                request->send(500, "text/plain", "Fehler beim Abrufen der Modbus-Einstellungen.");
            }
        } else {
            request->send(404, "text/plain", "Einstellungen nicht gefunden");
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
            if (saveToFile("//wlan-credentials.json", jsonConfig)) {
            request->redirect("/wlan-settings.html");
            } else {
            request->send(500, "text/plain", "Fehler beim Speichern der WLAN-Einstellungen.");
            }
}});

    server.on("/get-wlan-settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        const char* filename = "/wlan-credentials.json";
        char jsonData[128]; // Passen Sie die Größe entsprechend an
        if (readFromFile(filename, jsonData, sizeof(jsonData))) {
            request->send(200, "application/json", jsonData);
        } else {
            request->send(500, "text/plain", "Fehler beim Abrufen der WLAN-Einstellungen.");
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

    server.on("/get-status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    int rssi = WiFi.RSSI(); // WLAN-Empfangsstärke
    const char* modbusStatus = isConnectedToModbus() ? "Verbunden" : "Getrennt"; // Abhängig von Ihrer Implementierung
    char jsonResponse[512]; // Größe je nach Bedarf anpassen
    snprintf(jsonResponse, sizeof(jsonResponse), 
        // ... (bereits vorhandene JSON-Struktur) ...
        "\"rssi\":\"%d\","
        "\"modbusStatus\":\"%s\",",
        rssi, modbusStatus
    );
    request->send(200, "application/json", jsonResponse);
    });


    // Server starten
    server.begin();
}

void WebServer::handleClient() {
    // In dieser Implementierung ist nichts zu tun, da ESPAsyncWebServer asynchron ist
}


