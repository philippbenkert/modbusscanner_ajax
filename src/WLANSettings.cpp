#include "WLANSettings.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"

extern WebSocketHandler webSocketHandler;

void wlanSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);

    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);

        String wifiSSID;
        String wifiPassword;

        if (loadCredentials(wifiSSID, wifiPassword)) {

            char qrText[256];
            snprintf(qrText, sizeof(qrText), "WIFI:T:WPA2;S:%s;P:%s;H:false;;", wifiSSID.c_str(), wifiPassword.c_str());

            // Erstellen eines Hintergrundcontainers für den QR-Code
            lv_obj_t * qr_bg_container = lv_obj_create(content_container);
            lv_obj_set_size(qr_bg_container, 320, 320);
            lv_obj_align(qr_bg_container, LV_ALIGN_CENTER, 0, 0);

            // Stil für den Hintergrundcontainer mit abgerundeten Ecken
            static lv_style_t style_bg_rounded;
            lv_style_init(&style_bg_rounded);
            lv_style_set_radius(&style_bg_rounded, 20); // Abrunden der Ecken
            lv_style_set_bg_color(&style_bg_rounded, lv_color_white()); // Weißer Hintergrund
            lv_style_set_border_width(&style_bg_rounded, 0); // Kein Rand

            // Stil auf den Hintergrundcontainer anwenden
            lv_obj_add_style(qr_bg_container, &style_bg_rounded, 0);

            // Erstellen eines QR-Codes im Hintergrundcontainer
            lv_obj_t * qrcode = lv_qrcode_create(qr_bg_container, 300, lv_color_black(), lv_color_white());
            lv_obj_center(qrcode);

            lv_qrcode_update(qrcode, qrText, strlen(qrText));
        } else {
            drawContent("Fehler beim Laden der WLAN-Einstellungen.");
        }
    }
}