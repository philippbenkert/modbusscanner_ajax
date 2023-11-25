#include "ModbusSettings.h"
#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include <Preferences.h>

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

// Definieren Sie eine Struktur für die Benutzerdaten
struct ModbusSettingsData {
    lv_obj_t* ddlist_baudrate;
    lv_obj_t* ddlist_stopbits;
    lv_obj_t* ddlist_parity;
    lv_obj_t* ddlist_address;
    lv_obj_t* cb_default;
};

void saveModbusSettings(lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, bool defaultSettings);
// In the header file or at the top of the .cpp file
void loadModbusSettings(lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, lv_obj_t* cb_default);

void initialize_button_style() {
    if (!is_btn_style_initialized) {
        lv_style_init(&btn_style);
        lv_style_set_bg_color(&btn_style, lv_color_hex(0x00AEEF));
        lv_style_set_bg_opa(&btn_style, LV_OPA_COVER);
        is_btn_style_initialized = true; // Stellen Sie sicher, dass dies auf true gesetzt wird
    }
}

void setAndSaveDefaultModbusSettings() {
    preferences.begin("modbus_settings", false);
    preferences.putString("baudrate", DEFAULT_BAUDRATE);
    preferences.putString("stopbits", DEFAULT_STOPBITS);
    preferences.putString("parity", DEFAULT_PARITY);
    preferences.putString("address", DEFAULT_ADDRESS);
    preferences.end();
}

void updateDropdownsState(lv_obj_t* cb_default, lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address) {
    bool checked = lv_obj_get_state(cb_default) & LV_STATE_CHECKED;
    if (checked) {
        // Dropdowns deaktivieren
        lv_obj_add_state(ddlist_baudrate, LV_STATE_DISABLED);
        lv_obj_add_state(ddlist_stopbits, LV_STATE_DISABLED);
        lv_obj_add_state(ddlist_parity, LV_STATE_DISABLED);
        lv_obj_add_state(ddlist_address, LV_STATE_DISABLED);
    } else {
        // Dropdowns aktivieren
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

        // Dropdown für Baudrate
        lv_obj_t * ddlist_baudrate = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_baudrate, "9600\n19200\n38400\n57600\n115200");
        lv_obj_align(ddlist_baudrate, LV_ALIGN_TOP_LEFT, 10, 50);

        // Dropdown für Stopbits
        lv_obj_t * ddlist_stopbits = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_stopbits, "1\n2");
        lv_obj_align(ddlist_stopbits, LV_ALIGN_TOP_LEFT, 10, 90);

        // Dropdown für Parität
        lv_obj_t * ddlist_parity = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_parity, "None\nEven\nOdd");
        lv_obj_align(ddlist_parity, LV_ALIGN_TOP_LEFT, 10, 130);

        // Dropdown für Adresse
        lv_obj_t * ddlist_address = lv_dropdown_create(content_container);
        lv_dropdown_set_options(ddlist_address, "1\n2\n3\n4\n..."); // Fügen Sie hier Ihre Adressoptionen hinzu
        lv_obj_align(ddlist_address, LV_ALIGN_TOP_LEFT, 10, 170);
    
        // Checkbox für Standardeinstellungen erstellen
        lv_obj_t * cb_default = lv_checkbox_create(content_container);
        lv_checkbox_set_text(cb_default, "Standardeinstellungen");
        lv_obj_align(cb_default, LV_ALIGN_TOP_LEFT, 10, 210);

        // Initialisieren Sie die Datenstruktur nach der Erstellung der Dropdowns
        ModbusSettingsData* data = new ModbusSettingsData{
            ddlist_baudrate,
            ddlist_stopbits,
            ddlist_parity,
            ddlist_address,
            cb_default

        };

        // Laden der gespeicherten Einstellungen
        bool cb_default_state = lv_obj_get_state(cb_default) & LV_STATE_CHECKED; // Get the state of the checkbox
        loadModbusSettings(ddlist_baudrate, ddlist_stopbits, ddlist_parity, ddlist_address, cb_default);

        initialize_button_style();

        // Button "Verbinden" erstellen und Stil anwenden
        lv_obj_t * btn_connect = lv_btn_create(content_container);
        lv_obj_add_style(btn_connect, &btn_style, 0); // Stil auf den Button anwenden
        lv_obj_set_size(btn_connect, 100, 30);
        lv_obj_align(btn_connect, LV_ALIGN_TOP_MID, 80, 50);
        lv_obj_t * label_connect = lv_label_create(btn_connect);
        lv_label_set_text(label_connect, "Verbinden");
        lv_obj_align(label_connect, LV_ALIGN_CENTER, 0, 0);

        // Button "Trennen" erstellen und Stil anwenden
        lv_obj_t * btn_disconnect = lv_btn_create(content_container);
        lv_obj_add_style(btn_disconnect, &btn_style, 0); // Stil auf den Button anwenden
        lv_obj_set_size(btn_disconnect, 100, 30);
        lv_obj_align(btn_disconnect, LV_ALIGN_TOP_MID, 80, 90);
        lv_obj_t * label_disconnect = lv_label_create(btn_disconnect);
        lv_label_set_text(label_disconnect, "Trennen");
        lv_obj_align(label_disconnect, LV_ALIGN_CENTER, 0, 0);

        // Button "Speichern" erstellen und Stil anwenden
        lv_obj_t * btn_save = lv_btn_create(content_container);
        lv_obj_add_style(btn_save, &btn_style, 0);
        lv_obj_set_size(btn_save, 100, 30);
        lv_obj_align(btn_save, LV_ALIGN_TOP_MID, 80, 130); // Position anpassen
        lv_obj_t * label_save = lv_label_create(btn_save);
        lv_label_set_text(label_save, "Speichern");
        lv_obj_align(label_save, LV_ALIGN_CENTER, 0, 0);

        isMenuLocked = false;

        lv_obj_add_event_cb(btn_save, [](lv_event_t * e) {
        auto* data = static_cast<ModbusSettingsData*>(lv_event_get_user_data(e));
        defaultSettings = lv_obj_get_state(data->cb_default) & LV_STATE_CHECKED;
        saveModbusSettings(data->ddlist_baudrate, data->ddlist_stopbits, data->ddlist_parity, data->ddlist_address, defaultSettings);
        }, LV_EVENT_CLICKED, data);

        // Event-Callback für die Checkbox
        lv_obj_add_event_cb(cb_default, [](lv_event_t * e) {
        auto* cb_data = static_cast<ModbusSettingsData*>(lv_event_get_user_data(e));
        bool checked = lv_obj_get_state(cb_data->cb_default) & LV_STATE_CHECKED;

        updateDropdownsState(cb_data->cb_default, cb_data->ddlist_baudrate, cb_data->ddlist_stopbits, cb_data->ddlist_parity, cb_data->ddlist_address);

        if (checked) {
        // Standardwerte setzen
        lv_dropdown_set_selected(cb_data->ddlist_baudrate, 1); // Index für 19200
        lv_dropdown_set_selected(cb_data->ddlist_stopbits, 1); // Index für 2
        lv_dropdown_set_selected(cb_data->ddlist_parity, 0);   // Index für None
        lv_dropdown_set_selected(cb_data->ddlist_address, 0);  // Index für 1

        // Speichern der Standardwerte
        saveModbusSettings(cb_data->ddlist_baudrate, cb_data->ddlist_stopbits, cb_data->ddlist_parity, cb_data->ddlist_address, true);
        }
        // Speichern des neuen Status der Checkbox
        preferences.begin("modbus_settings", false);
        preferences.putBool("defaultSettings", checked);
        preferences.end();
        }, LV_EVENT_VALUE_CHANGED, data);
        }
    }

void saveModbusSettings(lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, bool defaultSettings) {
    preferences.begin("modbus_settings", false);
    char buf[64]; // Puffer für die ausgewählten Strings
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

void loadModbusSettings(lv_obj_t* ddlist_baudrate, lv_obj_t* ddlist_stopbits, lv_obj_t* ddlist_parity, lv_obj_t* ddlist_address, lv_obj_t* cb_default) {
    preferences.begin("modbus_settings", true);
    String baudrate = preferences.getString("baudrate", "9600");
    String stopbits = preferences.getString("stopbits", "1");
    String parity = preferences.getString("parity", "None");
    String address = preferences.getString("address", "1");
    int index = lv_dropdown_get_option_index(ddlist_baudrate, baudrate.c_str());
    lv_dropdown_set_selected(ddlist_baudrate, index);
    index = lv_dropdown_get_option_index(ddlist_stopbits, stopbits.c_str());
    lv_dropdown_set_selected(ddlist_stopbits, index);
    index = lv_dropdown_get_option_index(ddlist_parity, parity.c_str());
    lv_dropdown_set_selected(ddlist_parity, index);
    index = lv_dropdown_get_option_index(ddlist_address, address.c_str());
    lv_dropdown_set_selected(ddlist_address, index);
    // Laden und Setzen des Zustands der Checkbox
    bool defaultSettings = preferences.getBool("defaultSettings", false);
    if (defaultSettings) {
        lv_obj_add_state(cb_default, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(cb_default, LV_STATE_CHECKED);
    }
    updateDropdownsState(cb_default, ddlist_baudrate, ddlist_stopbits, ddlist_parity, ddlist_address);
    preferences.end();
}

