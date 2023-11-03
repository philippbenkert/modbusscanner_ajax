// TouchPanel.cpp
#include "TouchPanel.h"
#include "SDCardHandler.h"
#include "lvgl.h"  // LVGL-Bibliothek einbinden
#include "MenuDrawer.h"

#define TFT_WHITE 0xFFFF
#define TFT_BLACK 0x0000
#define TFT_PRIMARY 0x4A89DC
#define TFT_SECONDARY 0x967ADC
#define TFT_BACKGROUND 0x434A54
uint32_t lastActivityTime = 0;
bool isDisplayInStandby = false;
const uint32_t STANDBY_TIMEOUT = 30000; // 30 Sekunden

void* my_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
lv_fs_res_t my_close_cb(lv_fs_drv_t * drv, void * file_p);
lv_fs_res_t my_read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
lv_fs_res_t my_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
lv_fs_res_t my_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);

extern SDCardHandler sdCard;


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
    panel_cfg.bus_shared = false;
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
        lastActivityTime = millis();
        if (isDisplayInStandby) {
            display.setBrightness(255);  // Setzt die Hintergrundbeleuchtung auf maximale Helligkeit
            isDisplayInStandby = false;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void LGFX::init()
{
    begin();  // Wenn es eine begin() Methode gibt, die Sie aufrufen möchten
    
}
lgfx::Touch_FT5x06* LGFX::getTouchInstance()
{
    return &_touch_instance;
}

void init_lvgl_fs() {
    static lv_fs_drv_t fs_drv;  // Muss statisch sein
    lv_fs_drv_init(&fs_drv);
    fs_drv.letter = 'S';
    fs_drv.open_cb = my_open_cb;
    fs_drv.close_cb = my_close_cb;
    fs_drv.read_cb = my_read_cb;
    fs_drv.seek_cb = my_seek_cb;
    fs_drv.tell_cb = my_tell_cb;
    lv_fs_drv_register(&fs_drv);
}

static void my_lvgl_log_func(const char* log)
{
    // Sie können hier Serial.print oder eine andere Methode verwenden, um die Protokolle auszugeben.
    Serial.println(log);
}

void LGFX::lvgl_init()
{
    lv_init();
    lv_log_register_print_cb(my_lvgl_log_func);
    static lv_disp_draw_buf_t disp_buf;
    init_lvgl_fs();
// Puffer für den gesamten Bildschirm im PSRAM allokieren
    static lv_color_t *buf = (lv_color_t *)ps_malloc(TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t));
    if (buf == NULL) {
    // Fehlerbehandlung: Nicht genug Speicher im PSRAM oder PSRAM ist nicht verfügbar.
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
    lv_png_init();
    setupContentContainer();
    drawMenu();
    drawStatus();

}

void LGFX::lvgl_tick() {
}

void checkStandby() {
    if (!isDisplayInStandby && millis() - lastActivityTime > STANDBY_TIMEOUT) {
        // Hintergrundbeleuchtung ausschalten
        display.setBrightness(0);
        isDisplayInStandby = true;
    }
}
void checkTouch() {
    lv_task_handler();
}

void *my_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    File *file = new File(); // Dynamisch ein File-Objekt erstellen
    if (mode == LV_FS_MODE_WR) {
        *file = sdCard.open(path, FILE_WRITE);
    } else {
        *file = sdCard.open(path, FILE_READ);
    }
    if (file->available()) {
        return file;
    } else {
        delete file; // Speicher freigeben, wenn die Datei nicht geöffnet werden konnte
        return nullptr;
    }
}

lv_fs_res_t my_close_cb(lv_fs_drv_t * drv, void * file_p) {
    File *file = (File *)file_p;
    file->close();
    delete file; // Speicher freigeben
    return LV_FS_RES_OK;
}

lv_fs_res_t my_read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
    File *file = (File *)file_p;
    *br = file->read((uint8_t *)buf, btr);
    return LV_FS_RES_OK;
}

lv_fs_res_t my_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
    File *file = (File *)file_p;
    if (whence == LV_FS_SEEK_SET) {
        file->seek(pos);
    } else if (whence == LV_FS_SEEK_CUR) {
        file->seek(file->position() + pos);
    } else if (whence == LV_FS_SEEK_END) {
        file->seek(file->size() - pos);
    }
    return LV_FS_RES_OK;
}

lv_fs_res_t my_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
    File *file = (File *)file_p;
    *pos_p = file->position();
    return LV_FS_RES_OK;
}

