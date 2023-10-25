#pragma once
#include <AsyncWebSocket.h>

class WebSocketHandler {
public:
    bool isConnectedToModbus();
    String getFiles(const String& path);
    size_t getFreeSpace();
    public:
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    WebSocketHandler();
    void bindToServer(AsyncWebServer* server);
    String readFileContent(const String& path);
    String escapeJsonString(const String& input);
    String getWifiStrength();
    String getModbusStatus();
    String getFreeSpaceAsString();

private:
    AsyncWebSocket ws;
    File uploadFile;  // Deklarieren Sie hier die Variable
    void handleWebSocketConnect(AsyncWebSocketClient *client);
    void handleWebSocketDisconnect(AsyncWebSocketClient *client);
    void handleWebSocketData(AsyncWebSocketClient *client, uint8_t *data, size_t len);
    void handleGetStatus(AsyncWebSocketClient *client);
    void handleListDir(AsyncWebSocketClient *client, const String& msg);
    void handleDownload(AsyncWebSocketClient *client, const String& msg);
    void handleStartUpload(AsyncWebSocketClient *client, const String& msg);
    void handleUploadChunk(AsyncWebSocketClient *client, const String& msg);
    void handleEndUpload(AsyncWebSocketClient *client);
    void handleGetFreeSpace(AsyncWebSocketClient *client);
    void handleLoadContent(AsyncWebSocketClient *client, const String& url);
    void handleDeleteFile(AsyncWebSocketClient *client, const String& msg);


};

//inline void WebSocketHandler::bindToServer(AsyncWebServer* server) {
//    ws.onEvent(std::bind(&WebSocketHandler::onEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
//    server->addHandler(&ws);
//}
extern String getWlanSettingsAsJson();
