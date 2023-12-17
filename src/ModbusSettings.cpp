#include "ModbusSettings.h"
#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include <Preferences.h>
#include "WLANSettings.h"
#include "modbusScanner.h"

extern Preferences preferences;
extern WebSocketHandler webSocketHandler;
extern bool loadCredentials(String& ssid, String& password);
bool defaultSettings;
// Definieren der Standardwerte
const char* DEFAULT_BAUDRATE = "19200";
const char* DEFAULT_STOPBITS = "2";
const char* DEFAULT_PARITY = "None";
const char* DEFAULT_ADDRESS = "1";
static lv_style_t btn_style;
static bool is_btn_style_initialized = false;
String baudrate = preferences.getString("baudrate");
String stopbits = preferences.getString("stopbits");
String parity = preferences.getString("parity");
String address = preferences.getString("address");
bool isConnectedModbus = false;
String device = "Kein Geraet";
String addressOptions;
lv_obj_t* ddlist_device;

int modbusLogTemp;

// Definieren Sie eine Struktur für die Benutzerdaten
struct ModbusSettingsData {
    lv_obj_t* ddlist_baudrate;
    lv_obj_t* ddlist_stopbits;
    lv_obj_t* ddlist_parity;
    lv_obj_t* ddlist_address;
    lv_obj_t* cb_default;
};

static lv_style_t style_disabled;
static lv_style_t style_normal;

void updateToggleButtonLabel(lv_obj_t* btn) {
    if (btn == nullptr) return; // Sicherstellen, dass btn nicht null ist

    lv_obj_t* label = lv_obj_get_child(btn, 0); // Nehmen Sie an, dass das Label das erste Kind ist
    if (label != nullptr) {
        lv_label_set_text(label, isConnectedModbus ? "Trennen" : "Verbinden");
    }
}

void saveModbusSettings(lv_obj_t* ddlist_device, lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, bool defaultSettings);
// In the header file or at the top of the .cpp file
void loadModbusSettings(lv_obj_t* ddlist_device, lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, lv_obj_t* cb_default);

void initialize_button_style() {
    if (!is_btn_style_initialized) {
        lv_style_init(&btn_style);
        lv_style_set_bg_color(&btn_style, lv_color_hex(0x00AEEF));
        lv_style_set_bg_opa(&btn_style, LV_OPA_COVER);
        is_btn_style_initialized = true; // Stellen Sie sicher, dass dies auf true gesetzt wird
    }
}

void initialize_style_disabled() {
    if (!initialize_style_disabled) {
    lv_style_init(&style_disabled);
    lv_style_set_text_color(&style_disabled, lv_color_hex(0xC0C0C0)); // Graue Farbe
    lv_style_set_bg_color(&style_disabled, lv_color_hex(0xF0F0F0));   // Heller Hintergrund
    }
}

void initialize_style_normal() {
    if (!initialize_style_normal) {
    lv_style_init(&style_normal);
    lv_style_set_text_color(&style_disabled, lv_color_hex(0x000000)); // Graue Farbe
    lv_style_set_bg_color(&style_disabled, lv_color_hex(0xFFFFFF));   // Heller Hintergrund
    }
}

void initialize_styles() {

    initialize_style_disabled();
    initialize_style_normal();    
}

void toggleConnection(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);

    isConnectedModbus = !isConnectedModbus; // Umschalten des Verbindungsstatus

    if (isConnectedModbus) {
        // Code zum Herstellen der Modbus-Verbindung
    } else {
        // Code zum Trennen der Modbus-Verbindung
    }

    updateToggleButtonLabel(btn); // Aktualisieren des Button-Labels
}

void setAndSaveDefaultModbusSettings() {
    preferences.begin("modbus_settings", false);
    preferences.putString("baudrate", DEFAULT_BAUDRATE);
    preferences.putString("stopbits", DEFAULT_STOPBITS);
    preferences.putString("parity", DEFAULT_PARITY);
    preferences.putString("address", DEFAULT_ADDRESS);
    preferences.end();
}

    void updateDropdownsState(lv_obj_t* cb_default, lv_obj_t* ddlist_device, lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address) {
    bool checked = lv_obj_get_state(cb_default) & LV_STATE_CHECKED;
    if (checked) {
        // Dropdowns deaktivieren und Stil für deaktivierte Zustand anwenden
        
        lv_obj_add_state(ddlist_baudrate, LV_STATE_DISABLED);
        lv_obj_add_state(ddlist_stopbits, LV_STATE_DISABLED);
        lv_obj_add_state(ddlist_parity, LV_STATE_DISABLED);
        lv_obj_add_state(ddlist_address, LV_STATE_DISABLED);
    } else {
        // Dropdowns aktivieren und Stil für normalen Zustand anwenden
        lv_obj_clear_state(ddlist_baudrate, LV_STATE_DISABLED);
        lv_obj_clear_state(ddlist_stopbits, LV_STATE_DISABLED);
        lv_obj_clear_state(ddlist_parity, LV_STATE_DISABLED);
        lv_obj_clear_state(ddlist_address, LV_STATE_DISABLED);
        
    }
}

void modbusSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);

        initialize_styles();

        // Label für Geräte
        lv_obj_t * label_device = lv_label_create(content_container);
        lv_label_set_text(label_device, "Regler");
        lv_obj_align(label_device, LV_ALIGN_TOP_LEFT, 0, 10);

        // Dropdown für Geräte
    ddlist_device = lv_dropdown_create(content_container);

    std::vector<ModbusDeviceConfig> deviceConfigs = readModbusConfigs("/config/device.json");

    // Erstellen eines Strings für die Dropdown-Optionen
    String dropdownOptions;
    for (const auto& config : deviceConfigs) {
        if (!dropdownOptions.isEmpty()) {
            dropdownOptions += "\n";
        }
        dropdownOptions += config.geraetName.c_str();
    }

        // Setzen der Dropdown-Optionen
        lv_dropdown_set_options(ddlist_device, dropdownOptions.c_str());
        lv_obj_align(ddlist_device, LV_ALIGN_TOP_LEFT, 75, 0);
        lv_obj_add_style(ddlist_device, &style_no_border, 0);
        lv_obj_set_size(ddlist_device, 195, 30);

        // Label für Baudrate
        lv_obj_t * label_baudrate = lv_label_create(content_container);
        lv_label_set_text(label_baudrate, "Baudrate");
        lv_obj_align(label_baudrate, LV_ALIGN_TOP_LEFT, 0, 138);

        // Dropdown für Baudrate
        lv_obj_t * ddlist_baudrate = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_baudrate, "9600\n19200\n38400\n57600\n115200");
        lv_obj_align(ddlist_baudrate, LV_ALIGN_TOP_LEFT, 75, 130);
        lv_obj_add_style(ddlist_baudrate, &style_no_border, 0);
        lv_obj_set_size(ddlist_baudrate, 75, 30);

        // Label für Stopbits
        lv_obj_t * label_stopbits = lv_label_create(content_container);
        lv_label_set_text(label_stopbits, "Stopbits");
        lv_obj_align(label_stopbits, LV_ALIGN_TOP_LEFT, 155, 138);

        // Dropdown für Stopbits
        lv_obj_t * ddlist_stopbits = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_stopbits, "1\n2");
        lv_obj_align(ddlist_stopbits, LV_ALIGN_TOP_LEFT, 225, 130);
        lv_obj_add_style(ddlist_stopbits, &style_no_border, 0);
        lv_obj_set_size(ddlist_stopbits, 45, 30);

        // Label für Parität
        lv_obj_t * label_parity = lv_label_create(content_container);
        lv_label_set_text(label_parity, "Paritaet");
        lv_obj_align(label_parity, LV_ALIGN_TOP_LEFT, 0, 175);

        // Dropdown für Parität
        lv_obj_t * ddlist_parity = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_parity, "None\nEven\nOdd");
        lv_obj_align(ddlist_parity, LV_ALIGN_TOP_LEFT, 75, 167);
        lv_obj_add_style(ddlist_parity, &style_no_border, 0);
        lv_obj_set_size(ddlist_parity, 75, 30);

        // Label für Adresse
        lv_obj_t * label_address = lv_label_create(content_container);
        lv_label_set_text(label_address, "Adresse");
        lv_obj_align(label_address, LV_ALIGN_TOP_LEFT, 155, 175);

        // Dropdown für Adresse
        lv_obj_t * ddlist_address = lv_dropdown_create(content_container);
        

        for (int i = 1; i <= 254; ++i) {
            if (i > 1) {
                addressOptions += "\n";
            }
            addressOptions += String(i);
        }
        lv_dropdown_set_options(ddlist_address, addressOptions.c_str());
        lv_obj_align(ddlist_address, LV_ALIGN_TOP_LEFT, 225, 167);
        lv_obj_add_style(ddlist_address, &style_no_border, 0);
        lv_obj_set_size(ddlist_address, 45, 30);
        // Label für Standardeinstellungen
        lv_obj_t * label_default = lv_label_create(content_container);
        lv_label_set_text(label_default, "Standardeinstellungen");
        lv_obj_align(label_default, LV_ALIGN_TOP_LEFT, 0, 210);

        // Checkbox für Standardeinstellungen erstellen
        lv_obj_t * cb_default = lv_checkbox_create(content_container);
        lv_obj_align(cb_default, LV_ALIGN_TOP_LEFT, 250, 210);
        lv_checkbox_set_text(cb_default, "");
        ModbusSettingsData* data = new ModbusSettingsData{
            ddlist_baudrate,
            ddlist_stopbits,
            ddlist_parity,
            ddlist_address,
            cb_default

        };

        // Laden der gespeicherten Einstellungen
        bool cb_default_state = lv_obj_get_state(cb_default) & LV_STATE_CHECKED; // Get the state of the checkbox
        loadModbusSettings(ddlist_device, ddlist_baudrate, ddlist_stopbits, ddlist_parity, ddlist_address, cb_default);

        initialize_button_style();

        isConnectedModbus = false;

        // Toggle-Button erstellen
        lv_obj_t * btn_toggle = lv_btn_create(content_container);
        lv_obj_add_style(btn_toggle, &btn_style, 0);
        lv_obj_set_size(btn_toggle, 100, 30);
        lv_obj_align(btn_toggle, LV_ALIGN_TOP_MID, 85, 240); // Position anpassen
        lv_obj_t * label_toggle = lv_label_create(btn_toggle);
        lv_label_set_text(label_toggle, isConnectedModbus ? "Trennen" : "Verbinden");
        lv_obj_align(label_toggle, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(btn_toggle, toggleConnection, LV_EVENT_CLICKED, NULL);

        // Button "Speichern" erstellen und Stil anwenden
        lv_obj_t * btn_save = lv_btn_create(content_container);
        lv_obj_add_style(btn_save, &btn_style, 0);
        lv_obj_set_size(btn_save, 100, 30);
        lv_obj_align(btn_save, LV_ALIGN_TOP_MID, -20, 240); // Position anpassen
        lv_obj_t * label_save = lv_label_create(btn_save);
        lv_label_set_text(label_save, "Speichern");
        lv_obj_align(label_save, LV_ALIGN_CENTER, 0, 0);

        isMenuLocked = false;

        lv_obj_add_event_cb(btn_save, [](lv_event_t * e) {
        auto* data = static_cast<ModbusSettingsData*>(lv_event_get_user_data(e));
        defaultSettings = lv_obj_get_state(data->cb_default) & LV_STATE_CHECKED;
        saveModbusSettings(ddlist_device, data->ddlist_baudrate, data->ddlist_stopbits, data->ddlist_parity, data->ddlist_address, defaultSettings);
        }, LV_EVENT_CLICKED, data);

        // Event-Callback für die Checkbox
        lv_obj_add_event_cb(cb_default, [](lv_event_t * e) {
        auto* cb_data = static_cast<ModbusSettingsData*>(lv_event_get_user_data(e));
        bool checked = lv_obj_get_state(cb_data->cb_default) & LV_STATE_CHECKED;

        updateDropdownsState(cb_data->cb_default, ddlist_device, cb_data->ddlist_baudrate, cb_data->ddlist_stopbits, cb_data->ddlist_parity, cb_data->ddlist_address);

        if (checked) {
        lv_dropdown_set_selected(ddlist_device, 0);
        // Standardwerte setzen
        lv_dropdown_set_selected(cb_data->ddlist_baudrate, 1); // Index für 19200
        lv_dropdown_set_selected(cb_data->ddlist_stopbits, 1); // Index für 2
        lv_dropdown_set_selected(cb_data->ddlist_parity, 0);   // Index für None
        lv_dropdown_set_selected(cb_data->ddlist_address, 0);  // Index für 1

        // Speichern der Standardwerte
        saveModbusSettings(ddlist_device, cb_data->ddlist_baudrate, cb_data->ddlist_stopbits, cb_data->ddlist_parity, cb_data->ddlist_address, defaultSettings);
        }
        // Speichern des neuen Status der Checkbox
        preferences.begin("modbus_settings", false);
        preferences.putBool("defaultSettings", checked);
        preferences.end();
        }, LV_EVENT_VALUE_CHANGED, data);
        }
    }

void saveModbusSettings(lv_obj_t* ddlist_device, lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, bool defaultSettings) {
    preferences.begin("modbus_settings", false);
    char buf[64]; // Puffer für die ausgewählten Strings
    // Speichern des ausgewählten Geräts
    lv_dropdown_get_selected_str(ddlist_device, buf, sizeof(buf));
    preferences.putString("device", buf);
    lv_dropdown_get_selected_str(ddlist_baudrate, buf, sizeof(buf));
    preferences.putString("baudrate", buf);
    lv_dropdown_get_selected_str(ddlist_stopbits, buf, sizeof(buf));
    preferences.putString("stopbits", buf);
    lv_dropdown_get_selected_str(ddlist_parity, buf, sizeof(buf));
    preferences.putString("parity", buf);
    lv_dropdown_get_selected_str(ddlist_address, buf, sizeof(buf));
    preferences.putString("address", buf);
    // Speichern des Zustands der Checkbox
    preferences.putBool("defaultSettings", defaultSettings);
    preferences.end();
}

void loadModbusSettings(lv_obj_t* ddlist_device, lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, lv_obj_t* cb_default) {
    preferences.begin("modbus_settings", true);
    String baudrate = preferences.getString("baudrate", "9600");
    String stopbits = preferences.getString("stopbits", "1");
    String parity = preferences.getString("parity", "None");
    String address = preferences.getString("address", "1");
    String device = preferences.getString("device", "Kein Geraet");

    int index = lv_dropdown_get_option_index(ddlist_baudrate, baudrate.c_str());
    lv_dropdown_set_selected(ddlist_baudrate, index);
    index = lv_dropdown_get_option_index(ddlist_stopbits, stopbits.c_str());
    lv_dropdown_set_selected(ddlist_stopbits, index);
    index = lv_dropdown_get_option_index(ddlist_parity, parity.c_str());
    lv_dropdown_set_selected(ddlist_parity, index);
    index = lv_dropdown_get_option_index(ddlist_address, address.c_str());
    lv_dropdown_set_selected(ddlist_address, index);
    index = lv_dropdown_get_option_index(ddlist_device, device.c_str());
    lv_dropdown_set_selected(ddlist_device, index);
    // Laden und Setzen des Zustands der Checkbox
    bool defaultSettings = preferences.getBool("defaultSettings", false);
    if (defaultSettings) {
        lv_obj_add_state(cb_default, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(cb_default, LV_STATE_CHECKED);
    }
    updateDropdownsState(cb_default, ddlist_device, ddlist_baudrate, ddlist_stopbits, ddlist_parity, ddlist_address);

    preferences.end();
}