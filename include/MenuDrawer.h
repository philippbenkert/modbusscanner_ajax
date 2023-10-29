#ifndef MENU_DRAWER_H
#define MENU_DRAWER_H

#include <Arduino.h>
#include "lvgl.h"
#include "WebSocketHandler.h"

extern WebSocketHandler webSocketHandler;

// Strukturdefinition für das Menüelement
struct MenuItem {
    const char* iconPath;  // Pfad zum Iconsymbol
    void (*action)(lv_event_t*);  // Funktionssignatur für das Menüelement
};

// Funktionsdeklarationen
void drawMenu();
//void drawContent(String content);
void drawStatus();
void clearContentArea();
void setupContentContainer();

// Event-Handler-Funktionen für das Menü
void wlanSettingsFunction(lv_event_t * e);
void modbusSettingsFunction(lv_event_t * e);
void fileManagementFunction(lv_event_t * e);
void scanFunctionsFunction(lv_event_t * e);
extern lv_obj_t *content_container;
#endif
