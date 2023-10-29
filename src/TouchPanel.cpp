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

String iconPaths[] = {"/wifi.bin", "/modbus.bin", "/folder.bin", "/scan.bin"};

SDCardHandler sdCardHandler;
extern bool loadCredentials(String& ssid, String& password);

WebSocketHandler webSocketHandler;

LGFX::LGFX()
{
    // Bus-Konfiguration
    auto bus_cfg = _bus_instance.config();
    bus_cfg.freq_write = 20000000;    
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
    panel_cfg.readable = true;
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

static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
// Diese Funktion wird von LVGL aufgerufen, um einen Bereich des Displays zu aktualisieren
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    display.startWrite();
    display.setAddrWindow(area->x1, area->y1, w, h);
    display.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    display.endWrite();

    lv_disp_flush_ready(disp);
}

// Diese Funktion wird von LVGL aufgerufen, um den Touchstatus zu lesen
static void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;

    bool touched = display.getTouch(&touchX, &touchY);

    if(touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}
void LGFX::init()
{
    // Hier den Initialisierungscode für LGFX einfügen
    // Zum Beispiel:
    begin();  // Wenn es eine begin() Methode gibt, die Sie aufrufen möchten
    

}
lgfx::Touch_FT5x06* LGFX::getTouchInstance()
{
    return &_touch_instance;
}

static void my_lvgl_log_func(const char* log)
{
    // Sie können hier Serial.print oder eine andere Methode verwenden, um die Protokolle auszugeben.
    Serial.println(log);
}


void LGFX::lvgl_init()
{
    Serial.println("Starting LVGL initialization...");
    lv_init();
    Serial.println("LVGL initialized.");
    lv_log_register_print_cb(my_lvgl_log_func);
    

    static lv_disp_draw_buf_t disp_buf;

// Puffer für den gesamten Bildschirm im PSRAM allokieren
    static lv_color_t *buf = (lv_color_t *)ps_malloc(TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t));
    if (buf == NULL) {
    // Fehlerbehandlung: Nicht genug Speicher im PSRAM oder PSRAM ist nicht verfügbar.
    Serial.println("Fehler: Kann den Puffer im PSRAM nicht allokieren.");
    return; // oder eine andere geeignete Fehlerbehandlung
}

    lv_disp_draw_buf_init(&disp_buf, buf, NULL, TFT_WIDTH * TFT_HEIGHT);



    lv_disp_drv_init(&disp_drv);  // Treiberstruktur initialisieren
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;  // Setzen Sie Ihre Flush-Funktion als Callback
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);  // Und schließlich registrieren Sie den Treiber

    lv_indev_drv_init(&indev_drv);  // Treiberstruktur initialisieren
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;  // Setzen Sie Ihre Touch-Lesefunktion als Callback
    lv_indev_drv_register(&indev_drv);  // Und schließlich registrieren Sie den Treiber
    
    drawStatus();
    drawMenu();
}





void LGFX::lvgl_tick()
{
    //Serial.println("Starting LVGL tick...");
    //lv_tick_inc(1);  // 1ms tick
    //lv_task_handler();
    //Serial.println("LVGL tick completed.");
}

struct MenuItem {
    String text;
    int x, y, w, h;
    void (*action)(lv_event_t*);  // Beachten Sie die geänderte Funktionssignatur
};

MenuItem menuItems[] = {
    {"WLAN Einstellungen", 10, 10, 150, 40, wlanSettingsFunction},
    {"Modbus Einstellungen", 170, 10, 150, 40, modbusSettingsFunction},
    {"Datei-Management", 10, 60, 150, 40, fileManagementFunction},
    {"Scan-Funktionen", 170, 60, 150, 40, scanFunctionsFunction},
};

void drawMenu() {
    for (int i = 0; i < 4; i++) {
        MenuItem item = menuItems[i];

        // Button erstellen
        lv_obj_t * btn = lv_btn_create(lv_scr_act());
        lv_obj_set_pos(btn, item.x, item.y);
        lv_obj_set_size(btn, item.w, item.h);
        
        // Beschriftung zum Button hinzufügen
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, item.text.c_str());
        lv_obj_center(label);

        // Bild zum Button hinzufügen
        lv_obj_t * img = lv_img_create(btn);
        lv_img_set_src(img, iconPaths[i].c_str());  // Setzen Sie das Bild mit dem Dateipfad
        lv_obj_align(img, LV_ALIGN_LEFT_MID, 10, 0); // Beispielpositionierung
        
        // Ereignishandler zum Button hinzufügen
        lv_obj_add_event_cb(btn, item.action, LV_EVENT_CLICKED, NULL);
    }
}



void drawContent(String content) {
    
    lv_obj_t *content_container = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(content_container, lv_color_hex(0x4A89DC), 0);  // Zum Beispiel ein Blauton
    lv_obj_set_size(content_container, 200, 100);  // Beispielgröße
    lv_obj_align(content_container, LV_ALIGN_CENTER, 0, 0);

    // Erstellen Sie ein Textlabel innerhalb des Containers
    lv_obj_t *label = lv_label_create(content_container);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);  // Weiß
    lv_label_set_text(label, content.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void drawStatus() {
    
    String wifiStrength = webSocketHandler.getWifiStrength();
    String modbusStatus = webSocketHandler.getModbusStatus();
    String freeSpace = webSocketHandler.getFreeSpaceAsString();
    String status = "WLAN-Empfangsstärke: " + wifiStrength + "\n" +
                    "Modbus-Verbindungsstatus: " + modbusStatus + "\n" +
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
    lv_obj_center(label);

    Serial.println("drawStatus: End");
}



void clearContentArea() {
    lv_obj_clean(lv_scr_act());
}

void wlanSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
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

void modbusSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Modbus Einstellungen Inhalt...");
    }
}

void fileManagementFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Datei-Management Inhalt...");
    }
}

void scanFunctionsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Scan-Funktionen Inhalt...");
    }
}


void checkTouch() {
    lv_task_handler();
}