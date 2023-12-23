#include "WLANSettings.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include <Preferences.h>
#include "UIHandler.h"
#include "CommonDefinitions.h"
#include "UIHandler.h"
#include "ContentDrawer.h"

extern UIHandler uiHandler;
extern Preferences preferences;
extern WebSocketHandler webSocketHandler;

static lv_style_t style_bg_rounded;
static bool is_style_bg_rounded_initialized = false;
bool is_style_no_border_initialized = false;
static lv_style_t btn_style;
static bool is_btn_style_initialized = false;
static lv_style_t style_kb;
static bool is_style_kb_initialized = false;
bool isConnected = false;
extern bool shouldReconnect;

WLANSettings::WLANSettings() {
    // Initialisierungscodes (falls vorhanden)
}
void WLANSettings::setConnected(bool connected) {
    m_isConnected = connected;
}

bool WLANSettings::isConnected() const {
    return m_isConnected;
}
// In WLANSettings.cpp
void WLANSettings::wlanSettingsFunction(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
        wlanSettings.showAPMode(content_container); // Verwenden der globalen Instanz
        isMenuLocked = false;
    }
}

bool WLANSettings::loadSTACredentials(String &ssid, String &password) {
    preferences.begin("STA_credentials", true);
    bool hasCredentials = preferences.isKey("ssid") && preferences.isKey("password");
    if (hasCredentials) {
        ssid = preferences.getString("ssid");
        password = preferences.getString("password");
    }
    preferences.end();
    return hasCredentials;
}

void WLANSettings::saveCredentials(const char* ssid, const char* password) {
    preferences.begin("STA_credentials", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

void WLANSettings::showAPMode(lv_obj_t * parent) {
    String wifiSSID, wifiPassword;
    if (loadCredentials(wifiSSID, wifiPassword)) {
        char qrText[256];
        snprintf(qrText, sizeof(qrText), "WIFI:T:WPA2;S:%s;P:%s;H:false;;", wifiSSID.c_str(), wifiPassword.c_str());

        lv_obj_t * qr_bg_container = lv_obj_create(parent);
        lv_obj_set_size(qr_bg_container, 160, 160);
        lv_obj_align(qr_bg_container, LV_ALIGN_CENTER, 50, -50);
        lv_obj_clear_flag(qr_bg_container, LV_OBJ_FLAG_SCROLLABLE);

        uiHandler.initialize_style_bg_rounded();
        lv_obj_add_style(qr_bg_container, &style_bg_rounded, 0);

        lv_obj_t * qrcode = lv_qrcode_create(qr_bg_container, 150, lv_color_black(), lv_color_white());
        lv_obj_center(qrcode);
        lv_qrcode_update(qrcode, qrText, strlen(qrText));

        // UI-Elemente für SSID und Passwort
        loadSTACredentials(wifiSSID, wifiPassword);
        setupWifiUI(parent, wifiSSID, wifiPassword);
        
    } else {
        drawContent("Fehler beim Laden der WLAN-Einstellungen.");
    }
}

void WLANSettings::updateShouldReconnect(bool connect) {
    shouldReconnect = connect;
}

void WLANSettings::setupWifiUI(lv_obj_t * parent, const String &wifiSSID, const String &wifiPassword) {
    lv_obj_t * ssid_label = lv_label_create(parent);
    lv_label_set_text(ssid_label, "SSID:");
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 5, 180);
    uiHandler.initialize_style_no_border();
    lv_obj_t * ssid_input = lv_textarea_create(parent);
    lv_obj_set_size(ssid_input, 160, 30);
    lv_obj_align(ssid_input, LV_ALIGN_TOP_LEFT, 106, 170);
    lv_textarea_set_text(ssid_input, wifiSSID.c_str());
    lv_textarea_set_one_line(ssid_input, true); // Setzt das Textfeld auf eine Zeile
    lv_obj_add_style(ssid_input, &style_no_border, 0);
    lv_obj_t * password_label = lv_label_create(parent);
    lv_label_set_text(password_label, "Passwort:");
    lv_obj_align(password_label, LV_ALIGN_TOP_LEFT, 5, 215);
    lv_obj_t * password_input = lv_textarea_create(parent);
    lv_obj_set_size(password_input, 160, 30);
    lv_obj_align(password_input, LV_ALIGN_TOP_LEFT, 106, 205);
    lv_textarea_set_password_mode(password_input, true);
    lv_textarea_set_one_line(password_input, true); // Setzt das Textfeld auf eine Zeile
    lv_obj_add_style(password_input, &style_no_border, 0);
    actualPassword = wifiPassword;
    String passwordStars(wifiPassword.length(), '*');
    lv_textarea_set_text(password_input, passwordStars.c_str());
    lv_obj_add_event_cb(ssid_input, UIHandler::textarea_event_cb, LV_EVENT_CLICKED, &uiHandler);
    lv_obj_add_event_cb(password_input, UIHandler::textarea_event_cb, LV_EVENT_CLICKED, &uiHandler);
    uiHandler.initialize_btn_style();
    lv_obj_t * connect_btn = lv_btn_create(parent);
    lv_obj_set_size(connect_btn, 100, 28);
    lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_MID, 80, 5); // Position links vom Speichern-Button
    lv_obj_add_style(connect_btn, &btn_style, 0); // Stil auf den Button anwenden
    lv_obj_t * connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, isConnected() ? "Trennen" : "Verbinden");
    lv_obj_add_event_cb(connect_btn, UIHandler::connect_btn_event_cb, LV_EVENT_CLICKED, new std::pair<lv_obj_t*, lv_obj_t*>(ssid_input, password_input));

}

void WLANSettings::connectToWifi(const char* ssid, const char* password){
    WiFi.begin(ssid, password);
    wifiConnectAttempts = 0;
    while (wifiConnectAttempts < maxWifiConnectAttempts) {
        if (WiFi.status() == WL_CONNECTED) {
        setConnected(true);
        return;
}
        delay(1000);
        wifiConnectAttempts++;
    }
    setConnected(false);
}

void WLANSettings::disconnectWifi() {
    setConnected(false);
    WiFi.disconnect();
    wifiConnectAttempts = 0; // Zurücksetzen der Versuchszählung bei Trennung
}

void WLANSettings::updateConnectionButton(lv_obj_t* btn) {
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, isConnected() ? "Trennen" : "Verbinden");
}