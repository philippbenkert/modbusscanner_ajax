#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <LittleFS.h>

class WebServer {
    static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
   
public:
    WebServer();
    void begin();
    void handleClient();
    
    static bool isConnectedToModbus();
    static size_t getFreeSpace() {
        return LittleFS.totalBytes() - LittleFS.usedBytes();
    }
    static String getFiles(const String& path);
private:
    bool saveToFile(const char *path, const char *data);
    bool readFromFile(const char *path, char *data, size_t size);
    
    AsyncWebServer server;
};

