// TouchPanel.cpp
#include "TouchPanel.h"
#include "WebSocketHandlerHelpers.h"
#include "WebSocketHandler.h"
#include "SDCardHandler.h"
#include "lvgl.h"  // LVGL-Bibliothek einbinden

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
#define TFT_PRIMARY 0x4A89DC
#define TFT_SECONDARY 0x967ADC
#define TFT_BACKGROUND 0x434A54

String icons[] = {"/wifi.bmp", "/modbus.bmp", "/folder.bmp", "/scan.bmp"};

SDCardHandler sdCardHandler;
extern bool loadCredentials(String& ssid, String& password);

WebSocketHandler webSocketHandler;

LGFX::LGFX()
{
    // Bus-Konfiguration
    auto bus_cfg = _bus_instance.config();
    bus_cfg.freq_write = 16000000;    
    bus_cfg.pin_wr = 47;             
    bus_cfg.pin_rd = -1;             
    bus_cfg.pin_rs = 0;              
    bus_cfg.pin_d0 = 9;              
    bus_cfg.pin_d1 = 46;             
    bus_cfg.pin_d2 = 3;              
    bus_cfg.pin_d3 = 8;              
    bus_cfg.pin_d4 = 18;             
    bus_cfg.pin_d5 = 17;             
    bus_cfg.pin_d6 = 16;             
    bus_cfg.pin_d7 = 15;             
    _bus_instance.config(bus_cfg);   
    _panel_instance.setBus(&_bus_instance);

    // Panel-Konfiguration
    auto panel_cfg = _panel_instance.config();    
    panel_cfg.pin_cs = -1;  
    panel_cfg.pin_rst = 4;  
    panel_cfg.pin_busy = -1; 
    panel_cfg.panel_width = TFT_WIDTH;
    panel_cfg.panel_height = TFT_HEIGHT;
    panel_cfg.offset_x = 0;
    panel_cfg.offset_y = 0;
    panel_cfg.offset_rotation = 0;
    panel_cfg.dummy_read_pixel = 8;
    panel_cfg.dummy_read_bits = 1;
    panel_cfg.readable = false;
    panel_cfg.invert = true;
    panel_cfg.rgb_order = false;
    panel_cfg.dlen_16bit = false;
    panel_cfg.bus_shared = true;
    _panel_instance.config(panel_cfg);

    // Licht-Konfiguration
    auto light_cfg = _light_instance.config();    
    light_cfg.pin_bl = 45;              
    light_cfg.invert = false;           
    light_cfg.freq = 44100;           
    light_cfg.pwm_channel = 7;          
    _light_instance.config(light_cfg);
    _panel_instance.setLight(&_light_instance);

    // Touch-Konfiguration
    auto touch_cfg = _touch_instance.config();
    touch_cfg.x_min = 0;
    touch_cfg.x_max = 319;
    touch_cfg.y_min = 0;  
    touch_cfg.y_max = 479;
    touch_cfg.pin_int = 7;  
    touch_cfg.bus_shared = true; 
    touch_cfg.offset_rotation = 0;
    touch_cfg.i2c_port = 1;
    touch_cfg.i2c_addr = 0x38;
    touch_cfg.pin_sda = 6;   
    touch_cfg.pin_scl = 5;   
    touch_cfg.freq = 400000;  
    _touch_instance.config(touch_cfg);
    _panel_instance.setTouch(&_touch_instance);

    setPanel(&_panel_instance);
}

void LGFX::init()
{
    begin();
    delay(500);  // Warten Sie eine halbe Sekunde
    setFont(&fonts::Font4); // Setzen der Standard-Schriftart

    lvgl_init();  // LVGL initialisieren

    drawStatus();
    drawMenu();
}

lgfx::Touch_FT5x06* LGFX::getTouchInstance()
{
    return &_touch_instance;
}

void LGFX::lvgl_init()
{
    Serial.println("Starting LVGL initialization...");
    lv_init();
    Serial.println("LVGL initialized.");

    // Hier können Sie weitere LVGL-Initialisierungscodes hinzufügen, z.B. Display- und Eingabegeräte-Initialisierung
}

void LGFX::lvgl_tick()
{
    Serial.println("Starting LVGL tick...");
    lv_tick_inc(1);  // 1ms tick
    lv_task_handler();
    Serial.println("LVGL tick completed.");
}

struct MenuItem {
    String text;
    int x, y, w, h;
    void (*action)(lv_obj_t *, lv_event_t);
};

MenuItem menuItems[] = {
    {"WLAN Einstellungen", 10, 10, 150, 40, wlanSettingsFunction},
    {"Modbus Einstellungen", 170, 10, 150, 40, modbusSettingsFunction},
    {"Datei-Management", 10, 60, 150, 40, fileManagementFunction},
    {"Scan-Funktionen", 170, 60, 150, 40, scanFunctionsFunction},
};

void drawMenu() {
    Serial.println("Starting drawing menu...");
    File file;
    delay(500);  // Warten Sie eine halbe Sekunde

    int iconSpacing = display.width() / 4;  // Platzierung der Icons nebeneinander

    for (int i = 0; i < 4; i++) {
        file = SD.open(icons[i].c_str());
        if (file) {
            if (!display.drawBmp(&file, i * iconSpacing, 1)) {
                Serial.println("Error drawing BMP image: " + icons[i]);
            }
            file.close();
        } else {
            Serial.println("Error opening file: " + icons[i]);
        }
    }
    Serial.println("Menu drawing completed.");
}



void drawContent(String content) {
    Serial.println("drawContent: Start");
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, content.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void drawStatus() {
    Serial.println("drawStatus: Start");

    Serial.println("Getting WiFi strength...");
    String wifiStrength = webSocketHandler.getWifiStrength();
    Serial.println("WiFi strength obtained: " + wifiStrength);

    Serial.println("Getting Modbus status...");
    String modbusStatus = webSocketHandler.getModbusStatus();
    Serial.println("Modbus status obtained: " + modbusStatus);

    Serial.println("Getting free space...");
    String freeSpace = webSocketHandler.getFreeSpaceAsString();
    Serial.println("Free space obtained: " + freeSpace);

    String status = "WLAN-Empfangsstärke: " + wifiStrength + "\n" +
                    "Modbus-Verbindungsstatus: " + modbusStatus + "\n" +
                    "Freier Speicherplatz: " + freeSpace + " kByte";

    drawContent(status);

    Serial.println("drawStatus: End");
}


void clearContentArea() {
    lv_obj_clean(lv_scr_act());
}

void wlanSettingsFunction(lv_obj_t * obj, lv_event_t event) {
    if(lv_event_get_code(&event) == LV_EVENT_VALUE_CHANGED) {
        clearContentArea();
        String wifiSSID;
        String wifiPassword;

        if (loadCredentials(wifiSSID, wifiPassword)) {
            String content = "WLAN Einstellungen:\n";
            content += "SSID: " + wifiSSID + "\n";
            content += "Passwort: " + wifiPassword;
            drawContent(content);
        } else {
            drawContent("Fehler beim Laden der WLAN-Einstellungen.");
        }
    }
}

void modbusSettingsFunction(lv_obj_t * obj, lv_event_t event) {
    if(lv_event_get_code(&event) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Modbus Einstellungen Inhalt...");
    }
}

void fileManagementFunction(lv_obj_t * obj, lv_event_t event) {
    if(lv_event_get_code(&event) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Datei-Management Inhalt...");
    }
}

void scanFunctionsFunction(lv_obj_t * obj, lv_event_t event) {
    if(lv_event_get_code(&event) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Scan-Funktionen Inhalt...");
    }
}

void checkTouch() {
    // LVGL behandelt Touch-Ereignisse automatisch. Sie müssen nur Callbacks für Ihre Widgets hinzufügen.
}