#include "WLANSettings.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"

extern WebSocketHandler webSocketHandler;

void wlanSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        
        String wifiSSID;
        String wifiPassword;

        if (loadCredentials(wifiSSID, wifiPassword)) {

            char qrText[256];
            snprintf(qrText, sizeof(qrText), "WIFI:T:WPA2;S:%s;P:%s;H:false;;", wifiSSID.c_str(), wifiPassword.c_str());
lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
            // Erstellen eines QR-Codes mit LVGL-Funktionen
            lv_obj_t * qrcode = lv_qrcode_create(content_container, 300, lv_color_black(), lv_color_white());
            lv_obj_align(qrcode, LV_ALIGN_CENTER, 0, 0);
            lv_qrcode_update(qrcode, qrText, strlen(qrText));  // Die Länge des Textes hinzufügen

        } else {
            drawContent("Fehler beim Laden der WLAN-Einstellungen.");
        }
    }
}


