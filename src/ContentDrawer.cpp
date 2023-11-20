#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"

extern WebSocketHandler webSocketHandler;
extern bool loadCredentials(String& ssid, String& password);


void drawContent(String content) {
    lv_obj_t *label = lv_label_create(content_container);
    lv_label_set_text(label, content.c_str());
    lv_obj_set_style_text_font(label, &lv_font_saira_500, 0);
    lv_obj_set_width(label, lv_obj_get_width(content_container));
    lv_obj_center(label);
}

//void slider_event_handler(lv_event_t * e) {
//    lv_obj_t * slider = lv_event_get_target(e);
//    lv_obj_t * label_slider_value = reinterpret_cast<lv_obj_t *>(lv_event_get_user_data(e));
//    lv_label_set_text_fmt(label_slider_value, "%d", lv_slider_get_value(slider));
//}

void modbusSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();

        // Erstellen Sie einen neuen Stil für die Buttons
        static lv_style_t btn_style;
        lv_style_init(&btn_style);
        lv_style_set_bg_color(&btn_style, lv_color_hex(0x00AEEF)); // Setzen Sie die gewünschte Hintergrundfarbe
        lv_style_set_bg_opa(&btn_style, LV_OPA_COVER); // Deckkraft auf Maximum setzen

        // Button "Verbinden" erstellen und Stil anwenden
        lv_obj_t * btn_connect = lv_btn_create(content_container);
        lv_obj_add_style(btn_connect, &btn_style, 0); // Stil auf den Button anwenden
        lv_obj_set_size(btn_connect, 150, 30);
        lv_obj_align(btn_connect, LV_ALIGN_TOP_MID, 0, 10);
        lv_obj_t * label_connect = lv_label_create(btn_connect);
        lv_label_set_text(label_connect, "Verbinden");
        lv_obj_align(label_connect, LV_ALIGN_CENTER, 0, 0);

        // Button "Trennen" erstellen und Stil anwenden
        lv_obj_t * btn_disconnect = lv_btn_create(content_container);
        lv_obj_add_style(btn_disconnect, &btn_style, 0); // Stil auf den Button anwenden
        lv_obj_set_size(btn_disconnect, 150, 30);
        lv_obj_align(btn_disconnect, LV_ALIGN_TOP_MID, 0, 70);
        lv_obj_t * label_disconnect = lv_label_create(btn_disconnect);
        lv_label_set_text(label_disconnect, "Trennen");
        lv_obj_align(label_disconnect, LV_ALIGN_CENTER, 0, 0);

        // Platzhalter für erkanntes Gerät
        lv_obj_t * label_detected_device = lv_label_create(content_container);
        lv_label_set_text(label_detected_device, "Erkanntes Gerät: -"); // "-" als Platzhalter
        lv_obj_align(label_detected_device, LV_ALIGN_TOP_MID, 0, 180);
    }
}




void scanFunctionsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Scan-Funktionen Inhalt...");
    }
}