#include "WLANSettings.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include <Preferences.h>

extern Preferences preferences;
extern WebSocketHandler webSocketHandler;

static lv_style_t style_bg_rounded;
static bool is_style_bg_rounded_initialized = false;
static lv_style_t style_no_border;
static bool is_style_no_border_initialized = false;
static lv_style_t btn_style;
static bool is_btn_style_initialized = false;
static lv_style_t style_kb;
static bool is_style_kb_initialized = false;
int wifiConnectAttempts = 0;
const int maxWifiConnectAttempts = 3; // Maximale Anzahl von Verbindungsversuchen
String actualPassword;
bool isConnected = false;
extern bool shouldReconnect;

void showAPMode(lv_obj_t * parent);
void saveCredentials(const char* ssid, const char* password);
void textarea_event_cb(lv_event_t * e);
void createKeyboardPopup(lv_obj_t * parent, lv_obj_t * ta);
void keyboard_event_cb(lv_event_t * e);
void setupWifiUI(lv_obj_t * parent, const String &wifiSSID, const String &wifiPassword);
void connectToWifi(const char* ssid, const char* password);
void disconnectWifi();
void updateConnectionButton(lv_obj_t* btn);

void initialize_style_bg_rounded() {
    if (!is_style_bg_rounded_initialized) {
        lv_style_init(&style_bg_rounded);
        lv_style_set_radius(&style_bg_rounded, 5);
        lv_style_set_bg_color(&style_bg_rounded, lv_color_white());
        lv_style_set_border_width(&style_bg_rounded, 0);
        is_style_bg_rounded_initialized = true;
    }
}

void initialize_style_no_border() {
    if (!is_style_no_border_initialized) {
        lv_style_init(&style_no_border);
        lv_style_set_border_width(&style_no_border, 0);
        is_style_no_border_initialized = true;
    }
}

void initialize_btn_style() {
    if (!is_btn_style_initialized) {
        lv_style_init(&btn_style);
        lv_style_set_bg_color(&btn_style, lv_color_hex(0x00AEEF));
        lv_style_set_bg_opa(&btn_style, LV_OPA_COVER);
        is_btn_style_initialized = true;
    }
}

void initialize_style_kb() {
    if (!is_style_kb_initialized) {
        lv_style_init(&style_kb);
        lv_style_set_bg_color(&style_kb, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_text_color(&style_kb, lv_color_white());
        lv_style_set_border_width(&style_kb, 0);
        is_style_kb_initialized = true;
    }
}

void wlanSettingsFunction(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
        showAPMode(content_container);
        isMenuLocked = false;
    }
}

bool loadSTACredentials(String &ssid, String &password) {
    preferences.begin("STA_credentials", true);
    bool hasCredentials = preferences.isKey("ssid") && preferences.isKey("password");
    if (hasCredentials) {
        ssid = preferences.getString("ssid");
        password = preferences.getString("password");
    }
    preferences.end();
    return hasCredentials;
}

void saveCredentials(const char* ssid, const char* password) {
    preferences.begin("STA_credentials", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

void showAPMode(lv_obj_t * parent) {
    String wifiSSID, wifiPassword;
    if (loadCredentials(wifiSSID, wifiPassword)) {
        char qrText[256];
        snprintf(qrText, sizeof(qrText), "WIFI:T:WPA2;S:%s;P:%s;H:false;;", wifiSSID.c_str(), wifiPassword.c_str());

        lv_obj_t * qr_bg_container = lv_obj_create(parent);
        lv_obj_set_size(qr_bg_container, 160, 160);
        lv_obj_align(qr_bg_container, LV_ALIGN_CENTER, 50, -50);
        lv_obj_clear_flag(qr_bg_container, LV_OBJ_FLAG_SCROLLABLE);

        initialize_style_bg_rounded();
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

void updateShouldReconnect(bool connect) {
    shouldReconnect = connect;
}

static void connect_btn_event_cb(lv_event_t * e) {
    auto * user_data = static_cast<std::pair<lv_obj_t*, lv_obj_t*>*>(lv_event_get_user_data(e));
    lv_obj_t* ssid_input = user_data->first;
    lv_obj_t* password_input = user_data->second;

    const char* ssid = lv_textarea_get_text(ssid_input);
    const char* password = lv_textarea_get_text(password_input);

    auto * btn = lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(btn, 0);

    if (isConnected) {
        disconnectWifi();
        updateShouldReconnect(false);
        lv_label_set_text(label, "Verbinden");
    } else {
        // Ändern Sie den Text des Buttons zu "Trennen" vor dem Verbindungsversuch
        lv_label_set_text(label, "Trennen");

        if (strlen(ssid) > 0 && strlen(password) > 0) {
            // Speichern der Credentials, wenn beide Felder ausgefüllt sind
            saveCredentials(ssid, password);
            connectToWifi(ssid, password);
        } else {
            // Verwenden Sie die gespeicherten Anmeldeinformationen, wenn das Passwortfeld leer ist
            String savedSSID, savedPassword;
            if (loadSTACredentials(savedSSID, savedPassword)) {
                connectToWifi(savedSSID.c_str(), savedPassword.c_str());
            } else {
                // Fehlerbehandlung, falls keine gespeicherten Anmeldeinformationen vorhanden sind
                lv_label_set_text(label, "Verbinden");
                Serial.println("Keine gespeicherten WLAN-Anmeldeinformationen gefunden.");
                return;
            }
        }

        updateShouldReconnect(true);
        // Aktualisieren Sie den Text des Buttons basierend auf dem Ergebnis des Verbindungsversuchs
        lv_label_set_text(label, isConnected ? "Trennen" : "Verbinden");
    }
}


void setupWifiUI(lv_obj_t * parent, const String &wifiSSID, const String &wifiPassword) {
    lv_obj_t * ssid_label = lv_label_create(parent);
    lv_label_set_text(ssid_label, "SSID:");
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 5, 180);

    // Erstellen eines neuen Stils
    initialize_style_no_border();

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

    lv_obj_add_event_cb(ssid_input, textarea_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(password_input, textarea_event_cb, LV_EVENT_CLICKED, NULL);

    // Erstellen Sie einen neuen Stil für die Buttons
    initialize_btn_style();

    // Verbinden/Trennen Button
    lv_obj_t * connect_btn = lv_btn_create(parent);
    lv_obj_set_size(connect_btn, 100, 28);
    lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_MID, 80, 5); // Position links vom Speichern-Button
    lv_obj_add_style(connect_btn, &btn_style, 0); // Stil auf den Button anwenden
    lv_obj_t * connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, isConnected ? "Trennen" : "Verbinden");

    lv_obj_add_event_cb(connect_btn, connect_btn_event_cb, LV_EVENT_CLICKED, new std::pair<lv_obj_t*, lv_obj_t*>(ssid_input, password_input));

}

void connectToWifi(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    wifiConnectAttempts = 0;
    isConnected = true;
    while (wifiConnectAttempts < maxWifiConnectAttempts) {
        if (WiFi.isConnected()) {
            isConnected = true;
            return;
        }
        delay(1000);
        wifiConnectAttempts++;
    }

    isConnected = false;
    shouldReconnect = false;
    // Optional: Aktion, wenn die Verbindung nach maximalen Versuchen nicht hergestellt werden konnte
}

void disconnectWifi() {
    isConnected = false;
    WiFi.disconnect();
    wifiConnectAttempts = 0; // Zurücksetzen der Versuchszählung bei Trennung
}

void updateConnectionButton(lv_obj_t* btn) {
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    lv_label_set_text(label, isConnected ? "Trennen" : "Verbinden");
}

void createKeyboardPopup(lv_obj_t * parent, lv_obj_t * ta) {
    // Erstellen eines Fensters oder eines anderen Containers für die Tastatur
    lv_obj_t * kb_container = lv_obj_create(parent);
    lv_obj_set_size(kb_container, 300, 200);
    lv_obj_align(kb_container, LV_ALIGN_BOTTOM_MID, 0, -160);

    // Erstellen der Tastatur
    lv_obj_t * kb = lv_keyboard_create(kb_container);
    lv_obj_set_size(kb, 300, 200);
    lv_keyboard_set_textarea(kb, ta);

    // Anpassen des Tastaturstils
    initialize_style_kb();

    lv_obj_add_event_cb(kb, keyboard_event_cb, LV_EVENT_ALL, NULL);
}


void textarea_event_cb(lv_event_t * e) {
    lv_obj_t * ta = lv_event_get_target(e);
    createKeyboardPopup(lv_scr_act(), ta);
}

void keyboard_event_cb(lv_event_t * e) {
    lv_obj_t * kb = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_READY) {
        lv_obj_del(lv_obj_get_parent(kb)); // Schließen des Keyboards und des Containers
    }
}