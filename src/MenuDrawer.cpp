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
String iconPaths[] = {"/wifi.bin", "/modbus.bin", "/folder.bin", "/scan.bin"};

// Implementierung von drawMenu
std::vector<uint8_t*> imageBuffers;

MenuItem menuItems[] = {
    {"/wifi.bin", wlanSettingsFunction},
    {"/modbus.bin", modbusSettingsFunction},
    {"/folder.bin", fileManagementFunction},
    {"/scan.bin", scanFunctionsFunction},
};

void drawMenu() {
    int spaceBetweenItems = (TFT_WIDTH - 2 * screenPadding - numItems * iconSize) / (numItems - 1);
    uint8_t* buffers[numItems] = {nullptr};

    for (int i = 0; i < numItems; i++) {
        MenuItem item = menuItems[i];

        // Button erstellen
        lv_obj_t * btn = lv_btn_create(lv_scr_act());
        lv_obj_set_pos(btn, screenPadding + i * (iconSize + spaceBetweenItems), screenPadding);
        lv_obj_set_size(btn, iconSize, iconSize);

        // Bild zum Button hinzufügen (unter Verwendung von SDCardHandler)
        if (sdCard.isInitialized()) {
        File file = sdCard.open(item.iconPath, "r");
        if (file) {
            buffers[i] = new uint8_t[file.size()];
            file.read(buffers[i], file.size());
            imageBuffers.push_back(buffers[i]);  // Puffer zum Vektor hinzufügen

        
        lv_img_dsc_t img_desc;
        img_desc.data = buffers[i];
        img_desc.header.w = 50;  // Breite des Bildes (muss angegeben werden)
        img_desc.header.h = 50;  // Höhe des Bildes (muss angegeben werden)
        img_desc.header.cf = LV_IMG_CF_TRUE_COLOR;  // Farbformat (angepasst an das Bildformat)

        lv_obj_t * img = lv_img_create(btn);
        lv_img_set_src(img, &img_desc);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

        file.close();

    } else {
        Serial.println("Fehler beim Öffnen der Datei: " + String(item.iconPath));
    }
} else {
    Serial.println("SD-Karte nicht initialisiert.");
}
        // Ereignishandler zum Button hinzufügen
        lv_obj_add_event_cb(btn, item.action, LV_EVENT_CLICKED, NULL);
    }
}



void drawStatus() {
    
    String modbusStatus = webSocketHandler.getModbusStatus();
    String freeSpace = webSocketHandler.getFreeSpaceAsString();
    String status = "Modbus-Verbindungsstatus: " + modbusStatus + "\n" +
                    "Freier Speicherplatz: " + freeSpace + " kByte";

    // Container am unteren Bildschirmrand erstellen
    lv_obj_t * container = lv_obj_create(lv_scr_act());
    lv_obj_set_width(container, TFT_WIDTH); // Setzt die Breite des Containers auf die Bildschirmbreite
    lv_obj_set_height(container, 60); // Setzt eine feste Höhe für den Container, z.B. 60 Pixel
    lv_obj_align(container, LV_ALIGN_BOTTOM_MID, 0, 0); // Ausrichtung am unteren Bildschirmrand
    lv_obj_set_style_bg_color(container, lv_color_hex(0x4A89DC), 0);  // Zum Beispiel ein Blauton
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(container, 10, 0); // Setzt einen Innenabstand für den Container

    // Text zum Container hinzufügen
    lv_obj_t * label = lv_label_create(container);
    lv_label_set_text(label, status.c_str());
    lv_obj_set_style_text_font(label, &lv_font_saira_500, 0);  // Setzen Sie die Schriftart für das Label
    lv_obj_center(label);

    Serial.println("drawStatus: End");
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

    

