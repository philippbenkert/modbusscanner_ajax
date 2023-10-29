#pragma once

#include <Arduino.h>
#include "lvgl.h"
#include "MenuDrawer.h"

extern const lv_font_t lv_font_saira_500;

void drawContent(String content);
void wlanSettingsFunction(lv_event_t * e);
void modbusSettingsFunction(lv_event_t * e);
void fileManagementFunction(lv_event_t * e);
void scanFunctionsFunction(lv_event_t * e);
