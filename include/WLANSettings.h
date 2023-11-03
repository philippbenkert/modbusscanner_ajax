#ifndef WLANSETTINGS_H
#define WLANSETTINGS_H

#include "ContentDrawer.h"

// Deklarationen und Prototypen
extern bool loadCredentials(String& ssid, String& password);
void wlanSettingsFunction(lv_event_t * e);

#endif
