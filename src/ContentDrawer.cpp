#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"


extern WebSocketHandler webSocketHandler;
extern bool loadCredentials(String& ssid, String& password);

void wlanSettingsFunction(lv_event_t * e) {
    Serial.println("wlanSettingsFunction: Start");
    lv_obj_t * obj = lv_event_get_target(e);
    
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("Button clicked");
        clearContentArea();
        
        String wifiSSID;
        String wifiPassword;

        if (loadCredentials(wifiSSID, wifiPassword)) {
            Serial.println("Loaded credentials successfully");

            char qrText[256];
            snprintf(qrText, sizeof(qrText), "WIFI:T:WPA2;S:%s;P:%s;H:false;;", wifiSSID.c_str(), wifiPassword.c_str());
lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
            // Erstellen eines QR-Codes mit LVGL-Funktionen
            lv_obj_t * qrcode = lv_qrcode_create(content_container, 300, lv_color_black(), lv_color_white());
            lv_obj_align(qrcode, LV_ALIGN_CENTER, 0, 0);
            lv_qrcode_update(qrcode, qrText, strlen(qrText));  // Die Länge des Textes hinzufügen

        } else {
            Serial.println("Failed to load credentials");
            drawContent("Fehler beim Laden der WLAN-Einstellungen.");
        }
    }
    Serial.println("wlanSettingsFunction: End");
}



void drawContent(String content) {
    
    lv_obj_t *label = lv_label_create(content_container);
    lv_label_set_text(label, content.c_str());
    lv_obj_set_style_text_font(label, &lv_font_saira_500, 0);  // Setzen Sie die Schriftart für das Label
    lv_obj_center(label);
}

void modbusSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Modbus Einstellungen Inhalt...");
    }
}

void fileManagementFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Datei-Management Inhalt...");
    }
}

void scanFunctionsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Scan-Funktionen Inhalt...");
    }
}