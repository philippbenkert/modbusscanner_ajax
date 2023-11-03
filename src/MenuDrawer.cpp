#include "MenuDrawer.h"
#include "TouchPanel.h"  // und andere notwendige Header
#include "SDCardHandler.h"
#include "saira_500.c"
#include <vector>
#include <cstdint>

const int iconSize = 60;
const int screenPadding = 10;
const int numItems = 4;
lv_obj_t *content_container;

// Externe Verweise
extern SDCardHandler sdCard;
WebSocketHandler webSocketHandler;
String iconPaths[] = {"/wifi.png", "/modbus.png", "/folder.png", "/scan.png"};

// Implementierung von drawMenu
std::vector<uint8_t*> imageBuffers;

MenuItem menuItems[] = {
    {"/wifi.png", wlanSettingsFunction},
    {"/modbus.png", modbusSettingsFunction},
    {"/folder.png", fileManagementFunction},
    {"/scan.png", scanFunctionsFunction},
};

lv_obj_t* iconCache[numItems] = { NULL };

void drawMenu() {
    int spaceBetweenItems = (TFT_WIDTH - 2 * screenPadding - numItems * iconSize) / (numItems - 1);

    for (int i = 0; i < numItems; i++) {
        MenuItem item = menuItems[i];

        lv_obj_t * btn = lv_btn_create(lv_scr_act());
        lv_obj_set_pos(btn, screenPadding + i * (iconSize + spaceBetweenItems), screenPadding);
        lv_obj_set_size(btn, iconSize, iconSize);

        if(!iconCache[i]) {
            // Laden Sie das Symbol nur, wenn es noch nicht im Cache ist
            iconCache[i] = lv_img_create(btn);
            lv_img_set_src(iconCache[i], String("S:" + String(item.iconPath)).c_str());
        }

        lv_obj_align(iconCache[i], LV_ALIGN_CENTER, 0, 5);

        lv_obj_add_event_cb(btn, item.action, LV_EVENT_CLICKED, NULL);
    }
}

void drawStatus() {
    
    String modbusStatus = webSocketHandler.getModbusStatus();
    String freeSpace = webSocketHandler.getFreeSpaceAsString(); // Ich nehme an, dass hier "MB" bereits enthalten ist

    // Hauptcontainer
    lv_obj_t * container = lv_obj_create(lv_scr_act());
    lv_obj_set_width(container, TFT_WIDTH);
    lv_obj_set_height(container, 40); // reduzierte Höhe
    lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x4A89DC), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(container, 10, 0);
    lv_obj_set_style_pad_right(container, 10, 0);

    // Deaktivieren des Scrollbalkens für den Hauptcontainer
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);

    // Modbus-Status-Symbol
    lv_obj_t * modbusSymbol = lv_obj_create(container);
    lv_obj_set_size(modbusSymbol, 16, 16);
    lv_obj_set_style_bg_color(modbusSymbol, modbusStatus == "Verbunden" ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(modbusSymbol, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(modbusSymbol, 8, 0); // für einen Kreis
    lv_obj_align(modbusSymbol, LV_ALIGN_LEFT_MID, 20, 0); // 20 Pixel vom linken Rand

    lv_obj_t * modbusLabel = lv_label_create(container);
    lv_label_set_text(modbusLabel, "Modbus");
    lv_obj_align_to(modbusLabel, modbusSymbol, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    // Label für den freien Speicherplatz (ohne separates Symbol)
    lv_obj_t * spaceLabel = lv_label_create(container);
    lv_label_set_text(spaceLabel, freeSpace.c_str());
    lv_obj_align(spaceLabel, LV_ALIGN_RIGHT_MID, -5, 0);
}


void setupContentContainer() {
    
    static lv_style_t new_style;
    lv_style_init(&new_style);
    lv_style_set_text_font(&new_style, &lv_font_saira_500);
    lv_obj_add_style(lv_scr_act(), &new_style, 0);
    // Erstellt den Container für den Inhalt
    content_container = lv_obj_create(lv_scr_act());
    
    // Position und Größe des Containers setzen
    int x_pos = 10;  // Beginnen Sie am linken Rand
    int y_pos = 110; // Direkt unterhalb des Menüs
    int width = TFT_WIDTH - 20;  // Breite des Bildschirms minus 10 Pixel Abstand auf jeder Seite
    int height = TFT_HEIGHT - 180; // Der verbleibende Platz auf dem Bildschirm nach dem Menü

    lv_obj_set_pos(content_container, x_pos, y_pos);
    lv_obj_set_size(content_container, width, height);

    // Optional: Einige Stil-Einstellungen, falls Sie den Container hervorheben möchten
    lv_obj_set_style_bg_color(content_container, lv_color_hex(0xE0E0E0), 0); // Ein heller Hintergrund
    lv_obj_set_style_border_width(content_container, 2, 0);
    lv_obj_set_style_border_color(content_container, lv_color_hex(0x000000), 0);
    }

    void clearContentArea() {
    lv_obj_clean(content_container);
    }