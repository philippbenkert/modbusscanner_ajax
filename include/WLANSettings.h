#ifndef WLANSETTINGS_H
#define WLANSETTINGS_H

#include "ContentDrawer.h"

// Deklarationen und Prototypen
extern bool loadCredentials(String& ssid, String& password);
void wlanSettingsFunction(lv_event_t * e);
extern lv_style_t style_no_border;
extern bool is_style_no_border_initialized;
extern lv_obj_t* popup; 
extern void initialize_style_no_border();
#endif
