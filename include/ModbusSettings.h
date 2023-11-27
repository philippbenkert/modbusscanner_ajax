#ifndef MODBUS_SETTINGS_H
#define MODBUS_SETTINGS_H

#include <Arduino.h>  // Inkludiert die Arduino-Standardbibliothek, die die String-Klasse enthält
#include "lvgl.h"

// Deklaration der Funktion
void modbusSettingsFunction(lv_event_t * e);
void updateToggleButtonLabel(lv_obj_t* btn);
extern bool isConnectedModbus;
extern String device;

#endif // MODBUS_SETTINGS_H
