#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <LittleFS.h>
#include "lvgl.h"  // LVGL-Bibliothek einbinden

// Portrait
#define TFT_WIDTH   320
#define TFT_HEIGHT  480

class LGFX : public lgfx::LGFX_Device
{
private:
    lgfx::Panel_ST7796  _panel_instance;  // ST7796UI
    lgfx::Bus_Parallel8 _bus_instance;    // MCU8080 8B
    lgfx::Light_PWM     _light_instance;
    lgfx::Touch_FT5x06  _touch_instance;
    
public:
    LGFX();
    void init();
    lgfx::Touch_FT5x06* getTouchInstance(); // Methode, um das Touch-Objekt zu erhalten

    // LVGL spezifische Methoden
    void lvgl_init();  // Initialisiert LVGL
    void lvgl_tick();
      // Muss regelmäßig aufgerufen werden, um LVGL zu aktualisieren
    void checkTouch();
    void checkStandby();
};
extern LGFX display; // Externe Deklaration der display-Instanz
