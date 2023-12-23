#include "UIHandler.h"
extern WLANSettings wlanSettings;

void UIHandler::initialize_style_bg_rounded() {
    if (!is_style_bg_rounded_initialized) {
        lv_style_init(&style_bg_rounded);
        lv_style_set_radius(&style_bg_rounded, 5);
        lv_style_set_bg_color(&style_bg_rounded, lv_color_white());
        lv_style_set_border_width(&style_bg_rounded, 0);
        is_style_bg_rounded_initialized = true;
    }
}

void UIHandler::initialize_style_no_border() {
    if (!is_style_no_border_initialized) {
        lv_style_init(&wlanSettings.style_no_border);
        lv_style_set_border_width(&wlanSettings.style_no_border, 0);
        is_style_no_border_initialized = true;
    }
}

void UIHandler::initialize_btn_style() {
    if (!is_btn_style_initialized) {
        lv_style_init(&btn_style);
        lv_style_set_bg_color(&btn_style, lv_color_hex(0x00AEEF));
        lv_style_set_bg_opa(&btn_style, LV_OPA_COVER);
        is_btn_style_initialized = true;
    }
}

void UIHandler::initialize_style_kb() {
    if (!is_style_kb_initialized) {
        lv_style_init(&style_kb);
        lv_style_set_bg_color(&style_kb, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_text_color(&style_kb, lv_color_white());
        lv_style_set_border_width(&style_kb, 0);
        is_style_kb_initialized = true;
    }
}

void UIHandler::keyboard_event_cb(lv_event_t * e) {
    lv_obj_t * kb = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_READY) {
        lv_obj_del(lv_obj_get_parent(kb)); // Schließen des Keyboards und des Containers
    }
}

void UIHandler::createKeyboardPopup(lv_obj_t * parent, lv_obj_t * ta) {
    lv_obj_t * kb_container = lv_obj_create(parent);
    lv_obj_set_size(kb_container, 300, 200);
    lv_obj_align(kb_container, LV_ALIGN_BOTTOM_MID, 0, -160);
    lv_obj_t * kb = lv_keyboard_create(kb_container);
    lv_obj_set_size(kb, 300, 200);
    lv_keyboard_set_textarea(kb, ta);
    initialize_style_kb();
    lv_obj_add_event_cb(kb, keyboard_event_cb, LV_EVENT_ALL, NULL);
}

void UIHandler::textarea_event_cb(lv_event_t * e) {
    UIHandler* handler = static_cast<UIHandler*>(lv_event_get_user_data(e));
    if (!handler) {
        Serial.println("Handler is null");
        return;
    }

    lv_obj_t* ta = lv_event_get_target(e); 
    if (!ta) {
        Serial.println("Textarea is null");
        return;
    }
    handler->createKeyboardPopup(lv_scr_act(), ta);
}

void UIHandler::setWLANSettingsReference(WLANSettings* wlanSettings) {
    this->wlanSettingsRef = wlanSettings;
        Serial.println("WLANSettings-Referenz gesetzt."); // Debug-Ausgabe

}

void UIHandler::connect_btn_event_cb(lv_event_t * e) {
        UIHandler* handler = static_cast<UIHandler*>(lv_event_get_user_data(e));
        Serial.println("Connect-Button wurde geklickt.");

    auto * user_data = static_cast<std::pair<lv_obj_t*, lv_obj_t*>*>(lv_event_get_user_data(e));
    lv_obj_t* ssid_input = user_data->first;
    lv_obj_t* password_input = user_data->second;

    const char* ssid = lv_textarea_get_text(ssid_input);
    const char* password = lv_textarea_get_text(password_input);

    auto * btn = lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(btn, 0);

    if (handler->wlanSettingsRef->isConnected()) {
        handler->wlanSettingsRef->disconnectWifi();
        handler->wlanSettingsRef->updateShouldReconnect(false);
        lv_label_set_text(label, "Verbinden");
    } else {
        lv_label_set_text(label, "Trennen");

        if (strlen(ssid) > 0 && strlen(password) > 0) {
            handler->wlanSettingsRef->saveCredentials(ssid, password);
            handler->wlanSettingsRef->connectToWifi(ssid, password);
        } else {
            String savedSSID, savedPassword;
            if (handler->wlanSettingsRef->loadSTACredentials(savedSSID, savedPassword)) {
                handler->wlanSettingsRef->connectToWifi(savedSSID.c_str(), savedPassword.c_str());
            } else {
                lv_label_set_text(label, "Verbinden");
                Serial.println("Keine gespeicherten WLAN-Anmeldeinformationen gefunden.");
                return;
            }
        }
        handler->wlanSettingsRef->updateShouldReconnect(true);
        lv_label_set_text(label, handler->wlanSettingsRef->isConnected() ? "Trennen" : "Verbinden");
    }
}