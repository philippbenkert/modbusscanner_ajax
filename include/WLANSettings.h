#ifndef WLANSETTINGS_H
#define WLANSETTINGS_H

#include <Arduino.h>  // Inkludieren der Arduino-Standardbibliothek
#include "lvgl.h"



class WLANSettings {
public:
    WLANSettings(); // Konstruktor, falls benötigt
    static void wlanSettingsFunction(lv_event_t * e);
    void connectToWifi(const char* ssid, const char* password);
    void disconnectWifi();
    void showAPMode(lv_obj_t * parent);
    void setupWifiUI(lv_obj_t * parent, const String &wifiSSID, const String &wifiPassword);
    void updateConnectionButton(lv_obj_t* btn);
    bool isConnected() const; // Getter für den Verbindungsstatus
    void updateShouldReconnect(bool connect);
    void saveCredentials(const char* ssid, const char* password);
    bool loadSTACredentials(String &ssid, String &password);
    void setConnected(bool connected);
    lv_style_t style_no_border;

private:
    bool m_isConnected = false;
    int wifiConnectAttempts = 0;
    const int maxWifiConnectAttempts = 3;
    String actualPassword;
    // ... Andere private Mitglieder ...
};

extern WLANSettings wlanSettings;

#endif // WLANSETTINGS_H
