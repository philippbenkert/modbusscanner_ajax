#ifndef UIHANDLER_H
#define UIHANDLER_H

#include "lvgl.h"
#include "WLANSettings.h"

class UIHandler {
public:
    void initialize_style_bg_rounded();
    void initialize_style_no_border();
    void initialize_btn_style();
    void initialize_style_kb();
    void createKeyboardPopup(lv_obj_t * parent, lv_obj_t * ta);
    static void textarea_event_cb(lv_event_t * e);
    static void connect_btn_event_cb(lv_event_t * e);
    static void keyboard_event_cb(lv_event_t * e); // Machen Sie diese Methode statisch
    void setWLANSettingsReference(WLANSettings* wlanSettings);


private:
    lv_style_t style_bg_rounded;
    lv_style_t btn_style;
    lv_style_t style_kb;
    bool is_style_bg_rounded_initialized = false;
    bool is_style_no_border_initialized = false;
    bool is_btn_style_initialized = false;
    bool is_style_kb_initialized = false;
    WLANSettings* wlanSettingsRef = nullptr;
};

#endif // UIHANDLER_H
