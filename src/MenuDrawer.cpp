#include "MenuDrawer.h"
#include "TouchPanel.h"  // und andere notwendige Header
#include "SDCardHandler.h"
#include "saira_500.c"
#include <vector>
#include <cstdint>
#include "RTCControl.h"
#include "StatusDisplay.h"
#include "Process.h"

int activeButtonIndex = -1; // Anfangs ist kein Button aktiv

const int iconSize = 60;
const int screenPadding = 10;
const int numItems = 4;
lv_obj_t *content_container;
lv_obj_t* menuButtons[numItems]; // Array zum Speichern der Menü-Buttons
bool isInitialMenuLoad = true;
bool isMenuLocked = false;

// Externe Verweise
extern SDCardHandler sdCard;
WebSocketHandler webSocketHandler;

// Implementierung von drawMenu
std::vector<uint8_t*> imageBuffers;

MenuItem menuItems[] = {
    {"/wifi.png", wlanSettingsFunction},
    {"/modbus.png", modbusSettingsFunction},
    {"/folder.png", ProcessFunction},
    {"/scan.png", scanFunctionsFunction},
};

lv_obj_t* iconCache[numItems] = { NULL };

void setScreenBackgroundColor(lv_color_t color) {
    static lv_style_t style_bg; // Stellen Sie sicher, dass der Stil statisch oder global ist
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, color); 

    lv_obj_add_style(lv_scr_act(), &style_bg, 0);
}


void updateButtonStyles() {
    for (int i = 0; i < numItems; i++) {
        if (i == activeButtonIndex) {
            lv_obj_set_style_bg_color(menuButtons[i], lv_color_hex(0x00AEEF), 0); 
            lv_obj_set_style_shadow_opa(menuButtons[i], LV_OPA_TRANSP, 0); 
        } else {
            lv_obj_set_style_bg_color(menuButtons[i], lv_color_hex(0xFF8C00), 0); 
        }
    }
}

void buttonClickEventHandler(lv_event_t* e) {
    if (isMenuLocked) {
        return;
    }

    int clickedIndex = (int)lv_event_get_user_data(e);
    if (clickedIndex != activeButtonIndex) {
        activeButtonIndex = clickedIndex;
        updateButtonStyles();
        isMenuLocked = true;
        menuItems[clickedIndex].action(e);
    }
}
void triggerInitialButtonAction() {
    lv_event_t tempEvent;
    lv_obj_t* tempObj = lv_obj_create(lv_scr_act());
    tempEvent.target = tempObj;
    tempEvent.current_target = tempObj;
    tempEvent.code = LV_EVENT_CLICKED;
    menuItems[0].action(&tempEvent);
    lv_obj_del(tempObj);
}
void createButtonForMenuItem(int index, int spaceBetweenItems) {
    MenuItem item = menuItems[index];
    lv_obj_t* btn = lv_btn_create(lv_scr_act());
    lv_obj_set_pos(btn, screenPadding + index * (iconSize + spaceBetweenItems), screenPadding);
    lv_obj_set_size(btn, iconSize, iconSize);
    menuButtons[index] = btn;

    if (!iconCache[index]) {
        iconCache[index] = lv_img_create(btn);
        lv_img_set_src(iconCache[index], String("S:" + String(item.iconPath)).c_str());
        lv_obj_align(iconCache[index], LV_ALIGN_CENTER, 0, 5);
    }

    lv_obj_add_event_cb(btn, buttonClickEventHandler, LV_EVENT_CLICKED, (void*)index);
}
void drawMenu() {
    setScreenBackgroundColor(lv_color_make(0, 174, 239));
    int spaceBetweenItems = (TFT_WIDTH - 2 * screenPadding - numItems * iconSize) / (numItems - 1);

    for (int i = 0; i < numItems; i++) {
        createButtonForMenuItem(i, spaceBetweenItems);
    }
    updateButtonStyles();
    triggerInitialButtonAction();
}
void setupContentContainer() {
    static bool is_new_style_initialized = false;
    static lv_style_t new_style;

    if (!is_new_style_initialized) {
        lv_style_init(&new_style);
        lv_style_set_text_font(&new_style, LV_FONT_DEFAULT);
        is_new_style_initialized = true;
    }

    lv_obj_add_style(lv_scr_act(), &new_style, 0);
    content_container = lv_obj_create(lv_scr_act());
    int x_pos = 10;
    int y_pos = 110;
    int width = TFT_WIDTH - 20;
    int height = TFT_HEIGHT - 190;
    lv_obj_set_pos(content_container, x_pos, y_pos);
    lv_obj_set_size(content_container, width, height);
    lv_obj_set_style_bg_color(content_container, lv_color_hex(0xFF8C00), 0);
    lv_obj_set_style_border_width(content_container, 0, 0);
    }

    void clearContentArea() {
    if (content_container && lv_obj_is_valid(content_container)) {
        lv_obj_t * child;
        uint32_t i = 0;
        while ((child = lv_obj_get_child(content_container, i)) != NULL) {
            if (lv_obj_is_valid(child)) {
                lv_obj_remove_event_cb(child, NULL);
            }
            i++;
        }
        lv_obj_clean(content_container);
        dropdown_exists=false;
    }
    }