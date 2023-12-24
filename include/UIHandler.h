#ifndef UIHANDLER_H
#define UIHANDLER_H

#include "lvgl.h"
#include "WLANSettings.h"
#include <map>
#include <string>

class UIHandler {
public:
    void registerButtonName(lv_obj_t* button, const std::string& name);
    void initialize_style_bg_rounded();
    void initialize_style_no_border();
    void initialize_btn_style();
    void initialize_style_kb();
    void createKeyboardPopup(lv_obj_t * parent, lv_obj_t * ta);
    static void textarea_event_cb(lv_event_t * e);
    static void connect_btn_event_cb(lv_event_t * e);
    static void keyboard_event_cb(lv_event_t * e); // Machen Sie diese Methode statisch
    void setWLANSettingsReference(WLANSettings* wlanSettings);
    void updateConnectButtonLabel(const std::string& buttonName);
    void printButtonNames();
    void setSSIDInput(lv_obj_t* obj) { ssidInput = obj; }
    void setPasswordInput(lv_obj_t* obj) { passwordInput = obj; }
    lv_obj_t* getSSIDInput() const { return ssidInput; }
    lv_obj_t* getPasswordInput() const { return passwordInput; }
    static lv_obj_t* getButtonByName(const std::string& name) {
        auto it = buttonMap.find(name);
        if (it != buttonMap.end()) {
            return it->second;
        } else {
            return nullptr; // oder eine geeignete Fehlerbehandlung
        }
    }
    bool isObjectValid(lv_obj_t* obj) {
    // Überprüfen, ob das Objekt null ist
    if (obj == nullptr) {
        return false;
    }

    // Überprüfen, ob das Objekt ein gültiger LVGL-Typ ist
    if (!lv_obj_check_type(obj, &lv_obj_class)) {
        Serial.println("Object is not a valid LVGL object type.");
        return false;
    }

    // Überprüfen, ob das Objekt noch Teil des aktiven Bildschirms ist
    lv_obj_t* parent = lv_obj_get_parent(obj);
    return parent != nullptr;
    }

private:
    lv_style_t style_bg_rounded;
    lv_style_t btn_style;
    lv_style_t style_kb;
    bool is_style_bg_rounded_initialized = false;
    bool is_style_no_border_initialized = false;
    bool is_btn_style_initialized = false;
    bool is_style_kb_initialized = false;
    WLANSettings* wlanSettingsRef = nullptr;
    static std::map<std::string, lv_obj_t*> buttonMap;
    static std::string findButtonNameByObject(lv_obj_t* btnObject);
    lv_obj_t* ssidInput; // Für SSID-Eingabefeld
    lv_obj_t* passwordInput; // Für Passwort-Eingabefeld

};

#endif // UIHANDLER_H
