#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <LittleFS.h>


class WebServer {
    bool saveToFile(const char *path, const char *data);
    bool readFromFile(const char *path, char *data, size_t size);
    
    bool isConnectedToModbus();
    size_t getFreeSpace() {
        return LittleFS.totalBytes() - LittleFS.usedBytes();
    }

public:
    WebServer();
    void begin();
    void handleClient();


private:
    AsyncWebServer server;
};
