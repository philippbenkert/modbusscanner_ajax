#include "MenuDrawer.h"
#include "TouchPanel.h"  // und andere notwendige Header
#include "SDCardHandler.h"
#include "saira_500.c"
#include <vector>
#include <cstdint>
#include "RTCControl.h"
#include "StatusDisplay.h"
#include "FileManagement.h"

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
    {"/folder.png", fileManagementFunction},
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
            lv_obj_set_style_bg_color(menuButtons[i], lv_color_hex(0x00AEEF), 0); // Goldfarbe für aktiven Button
            lv_obj_set_style_shadow_opa(menuButtons[i], LV_OPA_TRANSP, 0); // Schatten entfernen für aktiven Button
        } else {
            lv_obj_set_style_bg_color(menuButtons[i], lv_color_hex(0xFF8C00), 0); 
        }
    }
}

void drawMenu() {
    setScreenBackgroundColor(lv_color_make(0, 174, 239)); // Gelber Hintergrund

    int spaceBetweenItems = (TFT_WIDTH - 2 * screenPadding - numItems * iconSize) / (numItems - 1);

    for (int i = 0; i < numItems; i++) {
        MenuItem item = menuItems[i];

        lv_obj_t * btn = lv_btn_create(lv_scr_act());
        lv_obj_set_pos(btn, screenPadding + i * (iconSize + spaceBetweenItems), screenPadding);
        lv_obj_set_size(btn, iconSize, iconSize);
        menuButtons[i] = btn; // Speichern des Buttons im Array

        if (!iconCache[i]) {
            iconCache[i] = lv_img_create(btn);
            lv_img_set_src(iconCache[i], String("S:" + String(item.iconPath)).c_str());
            lv_obj_align(iconCache[i], LV_ALIGN_CENTER, 0, 5);

        }

        lv_obj_add_event_cb(btn, [](lv_event_t * e) {
    if (isMenuLocked) {
        return; // Ignoriere Klicks, wenn das Menü gesperrt ist
    }

    int clickedIndex = (int)lv_event_get_user_data(e);
    if (clickedIndex != activeButtonIndex) {
        activeButtonIndex = clickedIndex;
        updateButtonStyles();

        // Rufen Sie die spezifische Funktion für den Button auf
        isMenuLocked = true;
        menuItems[clickedIndex].action(e);
    }
}, LV_EVENT_CLICKED, (void*)i);
    }

    // Aktualisieren der Button-Styles
    updateButtonStyles();
    // Erstellen eines temporären Event-Objekts
    lv_event_t tempEvent;

    // Initialisieren des Event-Objekts
    lv_obj_t *tempObj = lv_obj_create(lv_scr_act()); // Erstellen eines temporären LVGL-Objekts
    tempEvent.target = tempObj; // Setzen des Zielobjekts für das Event
    tempEvent.current_target = tempObj; // Setzen des aktuellen Zielobjekts
    tempEvent.code = LV_EVENT_CLICKED; // Setzen eines Event-Codes, z.B. LV_EVENT_CLICKED

    // Aufrufen der Funktion mit dem temporären Event
    scanFunctionsFunction(&tempEvent);

    // Aufräumen
    lv_obj_del(tempObj); // Löschen des temporären LVGL-Objekts
    
    }


void setupContentContainer() {
    static bool is_new_style_initialized = false; // Neue statische Variable
    static lv_style_t new_style;

    if (!is_new_style_initialized) {
        lv_style_init(&new_style);
        //lv_style_set_text_font(&new_style, &lv_font_saira_500);
        lv_style_set_text_font(&new_style, LV_FONT_DEFAULT); // Verwenden Sie LV_FONT_DEFAULT oder ein anderes eingebautes Font
        is_new_style_initialized = true; // Setzen Sie die Variable auf true, nachdem der Stil initialisiert wurde
    }

    lv_obj_add_style(lv_scr_act(), &new_style, 0);
    // Erstellt den Container für den Inhalt
    content_container = lv_obj_create(lv_scr_act());
    int x_pos = 10;  // Beginnen Sie am linken Rand
    int y_pos = 110; // Direkt unterhalb des Menüs
    int width = TFT_WIDTH - 20;  // Breite des Bildschirms minus 10 Pixel Abstand auf jeder Seite
    int height = TFT_HEIGHT - 190; // Der verbleibende Platz auf dem Bildschirm nach dem Menü
    lv_obj_set_pos(content_container, x_pos, y_pos);
    lv_obj_set_size(content_container, width, height);
    lv_obj_set_style_bg_color(content_container, lv_color_hex(0xFF8C00), 0); // Ein heller Hintergrund
    lv_obj_set_style_border_width(content_container, 0, 0);
    //lv_obj_set_style_border_color(content_container, lv_color_hex(0x000000), 0);
    }

    void clearContentArea() {
    if (content_container && lv_obj_is_valid(content_container)) {
        lv_obj_t * child;
        uint32_t i = 0;
        while ((child = lv_obj_get_child(content_container, i)) != NULL) {
            if (lv_obj_is_valid(child)) {
                // Entferne alle Event-Callbacks, die dem Kind zugeordnet sind
                lv_obj_remove_event_cb(child, NULL);
            }
            i++;
        }
        lv_obj_clean(content_container);
        dropdown_exists=false;
    }
    }