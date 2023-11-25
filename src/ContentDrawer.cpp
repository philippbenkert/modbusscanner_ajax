#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "ModbusSettings.h"

extern WebSocketHandler webSocketHandler;
extern bool loadCredentials(String& ssid, String& password);

void drawContent(String content) {
    clearContentArea();
    lv_obj_t *label = lv_label_create(content_container);
    lv_label_set_text(label, content.c_str());
    lv_obj_set_style_text_font(label, &lv_font_saira_500, 0);
    lv_obj_set_width(label, lv_obj_get_width(content_container));
    lv_obj_center(label);
}


void scanFunctionsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Scan-Funktionen Inhalt...");
        isMenuLocked = false;

    }
}