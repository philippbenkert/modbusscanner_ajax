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

extern int activeButtonIndex;
// Funktionsdeklarationen
void drawMenu();
void updateButtonStyles(); // Aktualisieren der Stile nach dem Erstellen der Menü-Buttons

//void drawContent(String content);
void drawStatus();
void clearContentArea();
void setupContentContainer();

// Neue Timer-Funktionsdeklarationen
void clear_all_timers();

// Event-Handler-Funktionen für das Menü
void modbusSettingsFunction(lv_event_t * e);
void ProcessFunction(lv_event_t * e);
void scanFunctionsFunction(lv_event_t * e);
extern lv_obj_t *content_container;
extern bool isMenuLocked;
extern bool dropdown_exists; // Globale Variable am Anfang des Codes
#endif
