#include "WebSocketHandler.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "modbusScanner.h"
#include "SDCardHandler.h"

bool isConnectedToModbus();
size_t getFreeSpace();

WebSocketHandler::WebSocketHandler() : ws("/ws") {}

bool WebSocketHandler::isConnectedToModbus() {
   return true; // Gibt immer "verbunden" zurück. Ersetzen Sie dies durch Ihre eigentliche Überprüfungslogik.
}

void WebSocketHandler::bindToServer(AsyncWebServer* server) {
    ws.onEvent(std::bind(&WebSocketHandler::onEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
    server->addHandler(&ws);
}

void WebSocketHandler::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        handleWebSocketConnect(client);
    } else if (type == WS_EVT_DISCONNECT) {
        handleWebSocketDisconnect(client);
    } else if (type == WS_EVT_DATA) {
        handleWebSocketData(client, data, len);
    }
}

void WebSocketHandler::handleWebSocketConnect(AsyncWebSocketClient *client) {
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
}

void WebSocketHandler::handleWebSocketDisconnect(AsyncWebSocketClient *client) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
}

void WebSocketHandler::handleWebSocketData(AsyncWebSocketClient *client, uint8_t *data, size_t len) {
    if (!len) return;

    String msg = String((char*)data).substring(0, len);
    Serial.printf("Received WebSocket message: %s\n", msg.c_str());

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, msg);
    String action = doc["action"];

    if (action == "httpRequest" && doc["data"]["url"].as<String>() == "/get-status") {
        handleGetStatus(client);
    } else if (action == "fetchFiles") {
        handleListDir(client, doc["data"]["path"].as<String>());
    } else if (action == "DOWNLOAD") {
        handleDownload(client, doc["data"]["path"].as<String>());
    } else if (action == "UPLOAD_STARTED") {
        handleStartUpload(client, doc["data"]["filename"].as<String>());
    } else if (action == "UPLOAD_CHUNK") {
        handleUploadChunk(client, doc["data"]["chunk"].as<String>());
    } else if (action == "END_UPLOAD") {
        handleEndUpload(client);
    } else if (action == "GET_FREE_SPACE") {
        handleGetFreeSpace(client);
    } else if (action == "loadContent") {
        handleLoadContent(client, doc["data"]["url"].as<String>());
    } else if (action == "deleteFile") {
    handleDeleteFile(client, doc["data"]["path"].as<String>());
    }
}

String WebSocketHandler::getWifiStrength() {
    int rssi = WiFi.RSSI();
    return String(rssi);
}

String WebSocketHandler::getModbusStatus() {
    return isConnectedToModbus() ? "Verbunden" : "Getrennt";
}

String WebSocketHandler::getFreeSpaceAsString() {
    if (!SD.begin()) {
        return "SD Fehler!";  // Oder einen anderen Fehler-String
    }

    uint64_t freeSpaceKB = SD.totalBytes() - SD.usedBytes(); // Freien Speicher in Bytes berechnen
    uint64_t freeSpaceMB = freeSpaceKB / (1024 * 1024);  // Von Bytes auf Megabytes umrechnen

    return String(freeSpaceMB) + " MB";
}

void WebSocketHandler::handleGetStatus(AsyncWebSocketClient *client) {
    String wifiStrength = getWifiStrength();
    String modbusStatus = getModbusStatus();
    String freeSpace = getFreeSpaceAsString();
    
    char jsonResponse[512];
    snprintf(jsonResponse, sizeof(jsonResponse), 
        "{"
        "\"success\":true, "
        "\"rssi\":\"%s\", "
        "\"modbusStatus\":\"%s\", "
        "\"freeSpace\":%s "
        "}",
        wifiStrength.c_str(), modbusStatus.c_str(), freeSpace.c_str()
    );
    client->text(jsonResponse);
}

void WebSocketHandler::handleListDir(AsyncWebSocketClient *client, const String& msg) {
    String path = msg.substring(8); // Annahme: Die Nachricht hat das Format "LIST_DIR/path/to/dir"
    if (path.length() == 0) path = "/logger/";
    client->text(getFiles(path));
}

void WebSocketHandler::handleDownload(AsyncWebSocketClient *client, const String& msg) {
    String path = msg.substring(9);
    if (LittleFS.exists(path.c_str())) {
        File file = LittleFS.open(path.c_str(), "r");
        String fileContent = file.readString();
        client->text("FILE_CONTENT:" + fileContent);
        file.close();
    } else {
        client->text("ERROR:File not found");
    }
}

void WebSocketHandler::handleStartUpload(AsyncWebSocketClient *client, const String& msg) {
    // Nehmen wir an, dass die Nachricht das Format "currentPath/filename" hat
    String fullPath = msg.substring(13); // Dies wird den vollständigen Pfad inklusive des Dateinamens extrahieren

    if (LittleFS.exists(fullPath.c_str())) {
        LittleFS.remove(fullPath.c_str());
    }
    uploadFile = LittleFS.open(fullPath, "w");
    client->text("UPLOAD_STARTED");
}

void WebSocketHandler::handleUploadChunk(AsyncWebSocketClient *client, const String& msg) {
    // Da die Nachricht direkt die Daten enthält, können wir sie direkt verwenden
    String chunkData = msg.substring(13);
    if (uploadFile) {
        uploadFile.write((uint8_t*)chunkData.c_str(), chunkData.length());
    }
}

void WebSocketHandler::handleEndUpload(AsyncWebSocketClient *client) {
    if (uploadFile) {
        uploadFile.close();
    }
    client->text("UPLOAD_COMPLETED");
}


void WebSocketHandler::handleDeleteFile(AsyncWebSocketClient *client, const String& msg) {
    String path = msg.substring(11); // Annahme: Die Nachricht hat das Format "DELETE_FILE/path/to/file"
    if (LittleFS.exists(path.c_str())) {
        LittleFS.remove(path.c_str());
        client->text("FILE_DELETED");
    } else {
        client->text("ERROR:File not found");
    }
}

void WebSocketHandler::handleGetFreeSpace(AsyncWebSocketClient *client) {
    size_t freeSpace = getFreeSpace();
    String jsonResponse = "{\"freeSpace\":" + String(freeSpace) + "}";
    client->text(jsonResponse);
}

void WebSocketHandler::handleLoadContent(AsyncWebSocketClient *client, const String& receivedUrl) {
    Serial.printf("Received loadContent request with URL: %s\n", receivedUrl.c_str());

    String content = readFileContent(receivedUrl);
    if (content.length() > 0) {
        String jsonResponse = "{\"success\":true, \"data\":\"" + escapeJsonString(content) + "\"}";
        client->text(jsonResponse);
    } else {
        String jsonResponse = "{\"success\":false, \"error\":\"Datei nicht gefunden oder leer.\"}";
        client->text(jsonResponse);
    }
}


String WebSocketHandler::escapeJsonString(const String& input) {
    String output = input;
    output.replace("\\", "\\\\");
    output.replace("\"", "\\\"");
    output.replace("/", "\\/");
    output.replace("\b", "\\b");
    output.replace("\f", "\\f");
    output.replace("\n", "\\n");
    output.replace("\r", "\\r");
    output.replace("\t", "\\t");
    return output;
}

String WebSocketHandler::readFileContent(const String& path) {
    if (!LittleFS.exists(path.c_str())) {
        return ""; // Datei existiert nicht
    }

    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        return ""; // Fehler beim Öffnen der Datei
    }

    String content = file.readString();
    file.close();
    return content;
}

String WebSocketHandler::getFiles(const String& path) {
    String output;
    File dir = LittleFS.open(path.c_str());
    for (File file = dir.openNextFile(); file; file = dir.openNextFile()) {
        if (output.length()) output += ',';
        
        if(file.isDirectory()){
            output += "{\"name\":\"" + String(file.name()) + "\",\"type\":\"directory\"}";
        } else {
            output += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + ",\"type\":\"file\"}";
        }
    }
    return "" + output + "";
}

size_t WebSocketHandler::getFreeSpace() {
        size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
    return freeBytes;
}
