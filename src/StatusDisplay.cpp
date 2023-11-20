// StatusDisplay.cpp
#include "StatusDisplay.h"
#include "WebSocketHandler.h"
#include "RTCControl.h"
#include "TouchPanel.h"  // und andere notwendige Header
#include "DateTimeHandler.h" // Inkluieren der neuen Datei

void settingsBtn_event_cb(lv_event_t * e);
void msgbox_event_cb(lv_event_t * e);
lv_obj_t * dateTimeLabel = nullptr;
lv_obj_t *dstSwitch = nullptr; // Definition of the switch object

// Hier fügen Sie alle externen Verweise ein, die im Status-Teil benötigt werden
extern WebSocketHandler webSocketHandler;
extern RTC_DS3231 rtc;

// Implementierung von drawStatus und allen anderen statusbezogenen Funktionen
bool dstEnabled; // Irgendwo in Ihrem Code definiert und gesetzt
bool isCurrentlyDST; // Diese Variable speichert den DST-Status

bool checkDST(DateTime now); // Funktionsprototyp-Deklaration
void adjustForDST();

lv_obj_t *dayRoller = nullptr;
lv_obj_t *monthRoller = nullptr;
lv_obj_t *yearRoller = nullptr;
lv_obj_t *hourRoller = nullptr;
lv_obj_t *minuteRoller = nullptr;

void datetime_set_event_cb(lv_event_t * e) {
    lv_obj_t * btnm = lv_event_get_target(e);
    uint32_t id = lv_btnmatrix_get_selected_btn(btnm);
    const char * txt = lv_btnmatrix_get_btn_text(btnm, id);

    if(strcmp(txt, "OK") == 0) {
        // Abrufen der Werte von den Rollern
        int day = lv_roller_get_selected(dayRoller) + 1; // +1, weil die Roller-Indizes bei 0 beginnen
        int month = lv_roller_get_selected(monthRoller) + 1; // +1 für denselben Grund
        int year = lv_roller_get_selected(yearRoller) + 2020; // Angenommen, die Jahre starten bei 2020 im Roller
        int hour = lv_roller_get_selected(hourRoller);
        int minute = lv_roller_get_selected(minuteRoller);

        // Erstellen eines neuen DateTime-Objekts mit den ausgewählten Werten
        DateTime newDateTime(year, month, day, hour, minute, 0);

        // RTC mit neuem Datum und neuer Uhrzeit aktualisieren
        rtc.adjust(newDateTime);

        // Schließen Sie das Einstellungsfenster
        lv_obj_del(lv_obj_get_parent(btnm));
    } else if(strcmp(txt, "Abbrechen") == 0) {
        // Schließen Sie das Einstellungsfenster ohne Änderungen
        lv_obj_del(lv_obj_get_parent(btnm));
    }
}

void update_datetime_label(lv_timer_t * timer) {
    if (dstEnabled) {
        adjustForDST(); // Überprüft und passt die Zeit an, wenn nötig
    }
    // 'label' ist das lv_obj_t* für das Datum- und Zeit-Label, das aktualisiert werden soll.
    lv_obj_t * label = static_cast<lv_obj_t*>(timer->user_data);
    
    // Holen Sie sich die aktuelle Zeit vom RTC
    String dateTimeString = getDateTimeStr();

    // Aktualisieren Sie das Label mit der neuen Zeit
    lv_label_set_text(label, dateTimeString.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -15); // Realign, falls die Größe des Textes sich ändert
}

void drawStatus() {
    
    String modbusStatus = webSocketHandler.getModbusStatus();
    String freeSpace = webSocketHandler.getFreeSpaceAsString();

    // Hauptcontainer, der nun beide Zeilen enthält
    lv_obj_t * statusContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_style_border_width(statusContainer, 0, 0);
    lv_obj_set_width(statusContainer, TFT_WIDTH-20);
    lv_obj_set_height(statusContainer, 65); // Platz für beide Zeilen
    lv_obj_align(statusContainer, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(statusContainer, lv_color_hex(0xFF8C00), 0);
    lv_obj_set_style_bg_opa(statusContainer, LV_OPA_COVER, 0);

    // Deaktivieren des Scrollbalkens für den Hauptcontainer
    lv_obj_set_scrollbar_mode(statusContainer, LV_SCROLLBAR_MODE_OFF);

    // Modbus-Status-Symbol
    lv_obj_t * modbusSymbol = lv_obj_create(statusContainer);
    lv_obj_set_size(modbusSymbol, 16, 16);
    lv_obj_set_style_bg_color(modbusSymbol, modbusStatus == "Verbunden" ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(modbusSymbol, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(modbusSymbol, 8, 0); // für einen Kreis
    lv_obj_align(modbusSymbol, LV_ALIGN_LEFT_MID, 5, 15); // 20 Pixel vom linken Rand, 20 Pixel oberhalb der Mitte

    lv_obj_t * modbusLabel = lv_label_create(statusContainer);
    lv_label_set_text(modbusLabel, "Modbus");
    lv_obj_align(modbusLabel, LV_ALIGN_LEFT_MID, 25, 15);

    // Label für den freien Speicherplatz (ohne separates Symbol)
    lv_obj_t * spaceLabel = lv_label_create(statusContainer);
    lv_label_set_text(spaceLabel, freeSpace.c_str());
    lv_obj_align(spaceLabel, LV_ALIGN_RIGHT_MID, -5, 15);

    // Label für Datum und Uhrzeit
    dateTimeLabel = lv_label_create(statusContainer);
    String dateTimeString = getDateTimeStr();
    lv_label_set_text(dateTimeLabel, dateTimeString.c_str());
    lv_obj_align(dateTimeLabel, LV_ALIGN_TOP_MID, 0, 25); // Anpassung der Y-Position

    // Einstellschlüssel-Symbol als Button ohne Hintergrund (transparent)
    lv_obj_t * settingsBtn = lv_btn_create(statusContainer);
    // Entfernen des Hintergrundstils vom Button, um ihn transparent zu machen
    lv_obj_remove_style_all(settingsBtn); // Entfernen aller Stile, die vom Container geerbt wurden
    lv_obj_set_size(settingsBtn, 25, 25);
    lv_obj_align(settingsBtn, LV_ALIGN_CENTER, 70, -15); // Rechts neben dem Datum/Uhrzeit-Label   
    // Hinzufügen des Einstellungssymbols zum Button
    lv_obj_t * settingsSymbol = lv_label_create(settingsBtn);
    lv_label_set_text(settingsSymbol, LV_SYMBOL_SETTINGS);
    lv_obj_center(settingsSymbol);

    // Event-Handler für den Einstellschlüssel
    lv_obj_add_event_cb(settingsBtn, settingsBtn_event_cb, LV_EVENT_CLICKED, NULL);

    // Timer für die Aktualisierung des Datum/Uhrzeit-Labels einrichten
    setup_datetime_update(dateTimeLabel); // Pass dateTimeLabel as an argument

}

void dst_switch_event_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    bool is_checked = lv_obj_get_state(obj) & LV_STATE_CHECKED;

    // Speichern Sie den neuen Zustand von dstEnabled
    dstEnabled = is_checked;    saveDSTEnabled(); // Speichere den neuen Zustand

    // Möglicherweise müssen Sie hier die Zeit sofort anpassen
    adjustForDST();
}

// Diese Funktion wird aufgerufen, wenn das Einstellschlüssel-Symbol angeklickt wird
    void settingsBtn_event_cb(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);

    // Erstellen Sie einen neuen Container für die Datums- und Zeiteinstellungen
    lv_obj_t * datetimeContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(datetimeContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_center(datetimeContainer);
    lv_obj_set_style_bg_color(datetimeContainer, lv_color_hex(0x00AEEF), 0); // Beispielfarbe
    lv_obj_set_style_border_width(datetimeContainer, 0, 0);
    lv_obj_set_style_radius(datetimeContainer, 10, 0); // Abgerundete Ecken

    // Erstellen und positionieren Sie die Überschrift
    lv_obj_t * label = lv_label_create(datetimeContainer);
    lv_label_set_text(label, "Datum und Uhrzeit einstellen");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);  // Positionieren Sie die Überschrift oben im Container
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_STATE_DEFAULT); // Beispiel Schriftart

    // Roller-Designanpassungen
    static lv_style_t roller_selected_style;
    lv_style_init(&roller_selected_style);
    lv_style_set_bg_color(&roller_selected_style, lv_color_hex(0x00AEEF)); // Blauer Hintergrund für ausgewählte Zahlen
    lv_style_set_text_color(&roller_selected_style, lv_color_hex(0xFFFFFF)); // Weißer Text

    // Erstellen Sie Roller für Tag, Monat, Jahr, Stunde und Minute
    dayRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(dayRoller, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(dayRoller, LV_ALIGN_CENTER, -130, -50);
    lv_obj_set_style_bg_color(dayRoller, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Hintergrundfarbe
    lv_obj_set_style_text_font(dayRoller, &lv_font_montserrat_16, LV_STATE_DEFAULT); // Text Schriftart
    lv_obj_set_style_text_color(dayRoller, lv_color_hex(0x000000), LV_PART_MAIN); // Schwarzer Text
    lv_obj_add_style(dayRoller, &roller_selected_style, LV_PART_SELECTED);

    monthRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(monthRoller, "Jan\nFeb\nMar\nApr\nMay\nJun\nJul\nAug\nSep\nOct\nNov\nDec", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(monthRoller, LV_ALIGN_CENTER, -70, -50);
    lv_obj_set_style_bg_color(monthRoller, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Hintergrundfarbe
    lv_obj_set_style_text_font(monthRoller, &lv_font_montserrat_16, LV_STATE_DEFAULT); // Text Schriftart
    lv_obj_set_style_text_color(monthRoller, lv_color_hex(0x000000), LV_PART_MAIN); // Schwarzer Text
    lv_obj_add_style(monthRoller, &roller_selected_style, LV_PART_SELECTED);

    yearRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(yearRoller, "2020\n2021\n2022\n2023\n2024\n2025\n2026\n2027\n2028\n2029\n2030", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(yearRoller, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_bg_color(yearRoller, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Hintergrundfarbe
    lv_obj_set_style_text_font(yearRoller, &lv_font_montserrat_16, LV_STATE_DEFAULT); // Text Schriftart
    lv_obj_set_style_text_color(yearRoller, lv_color_hex(0x000000), LV_PART_MAIN); // Schwarzer Text
    lv_obj_add_style(yearRoller, &roller_selected_style, LV_PART_SELECTED);

    hourRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(hourRoller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(hourRoller, LV_ALIGN_CENTER, 70, -50);
    lv_obj_set_style_bg_color(hourRoller, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Hintergrundfarbe
    lv_obj_set_style_text_font(hourRoller, &lv_font_montserrat_16, LV_STATE_DEFAULT); // Text Schriftart
    lv_obj_set_style_text_color(hourRoller, lv_color_hex(0x000000), LV_PART_MAIN); // Schwarzer Text
    lv_obj_add_style(hourRoller, &roller_selected_style, LV_PART_SELECTED);

    minuteRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(minuteRoller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(minuteRoller, LV_ALIGN_CENTER, 130, -50);
    lv_obj_set_style_bg_color(minuteRoller, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Hintergrundfarbe
    lv_obj_set_style_text_font(minuteRoller, &lv_font_montserrat_16, LV_STATE_DEFAULT); // Text Schriftart
    lv_obj_set_style_text_color(minuteRoller, lv_color_hex(0x000000), LV_PART_MAIN); // Schwarzer Text
    lv_obj_add_style(minuteRoller, &roller_selected_style, LV_PART_SELECTED);

    DateTime now = getRTCDateTime(); // Obtain the current DateTime from RTC
    setDateTimeRollersToCurrent(); // Pass the 'now' as argument

    // DST-Switch-Designanpassungen
    static lv_style_t dst_switch_style;
    lv_style_init(&dst_switch_style);
    lv_style_set_bg_color(&dst_switch_style, lv_color_hex(0xFF8C00)); // Hintergrundfarbe des Switch-Indikators

    lv_obj_t *dstSwitch = lv_switch_create(datetimeContainer);
    lv_obj_align(dstSwitch, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_event_cb(dstSwitch, dst_switch_event_cb, LV_EVENT_CLICKED, NULL);
    if (dstEnabled) {
        lv_obj_add_state(dstSwitch, LV_STATE_CHECKED); // Turn the switch "on"
    } else {
        lv_obj_clear_state(dstSwitch, LV_STATE_CHECKED); // Turn the switch "off"
    }
    lv_obj_add_style(dstSwitch, &dst_switch_style, LV_PART_INDICATOR);
    lv_obj_add_style(dstSwitch, &dst_switch_style, LV_PART_KNOB);

    lv_obj_set_style_bg_color(dstSwitch, lv_color_hex(0xFF8C00), LV_PART_INDICATOR); // Farbe des Indikators
    lv_obj_set_style_bg_color(dstSwitch, lv_color_hex(0xFFFFFF), LV_PART_KNOB); // Farbe des Knopfes

    // Erstellen und positionieren Sie die Überschrift
    lv_obj_t * label1 = lv_label_create(datetimeContainer);
    lv_label_set_text(label1, "Sommer-/Winterumschaltung");
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 50);  // Positionieren Sie die Überschrift oben im Container

    // Hinzufügen der Bestätigungs- und Abbrechen-Schaltflächen
    lv_obj_t * btnm = lv_btnmatrix_create(datetimeContainer);
    lv_obj_set_style_bg_color(btnm, lv_color_hex(0xFF8C00), LV_PART_ITEMS); // Farbe der Buttons
    lv_obj_set_style_text_color(btnm, lv_color_hex(0x00AEEF), LV_PART_MAIN); // Weißer Text

    // Anpassen des Hintergrunds der Buttons in der Buttonmatrix
    static lv_style_t btnm_style;
    lv_style_init(&btnm_style);
    lv_style_set_bg_color(&btnm_style, lv_color_hex(0x00AEEF)); // Blauer Hintergrund
    lv_style_set_text_color(&btnm_style, lv_color_hex(0xFFFFFF)); // Weißer Text
    lv_obj_add_style(btnm, &btnm_style, LV_PART_MAIN);

    static const char * btnm_map[] = {"OK", "Abbrechen", ""};
    lv_btnmatrix_set_map(btnm, btnm_map);
    lv_obj_set_size(btnm, 240, 60);

    lv_btnmatrix_set_btn_ctrl_all(btnm, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_btn_width(btnm, 0, 2);
    lv_btnmatrix_set_btn_width(btnm, 1, 2);

    lv_obj_align(btnm, LV_ALIGN_CENTER, 0, 150);

    // Event-Callbacks für die Schaltflächen hinzufügen
    lv_obj_add_event_cb(btnm, datetime_set_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// Event-Handler für die Nachrichtenbox
void msgbox_event_cb(lv_event_t * e) {
    lv_obj_t * msgbox = (lv_obj_t *)lv_event_get_user_data(e);  // Zugriff auf die msgbox über Benutzerdaten
    lv_obj_t * btn = lv_event_get_target(e); // Das tatsächliche Objekt, das das Ereignis ausgelöst hat
    const char * txt = lv_msgbox_get_active_btn_text(msgbox); // Wir verwenden das Nachrichtenbox-Objekt hier
    if(txt && strcmp(txt, "OK") == 0) {
        lv_msgbox_close(msgbox); // Schließen der msgbox
        // Code zum Speichern der Einstellungen hier...
    }
}
