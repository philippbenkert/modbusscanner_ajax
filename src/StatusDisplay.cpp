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
    lv_obj_set_width(statusContainer, TFT_WIDTH-10);
    lv_obj_set_height(statusContainer, 65); // Platz für beide Zeilen
    lv_obj_align(statusContainer, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(statusContainer, lv_color_hex(0x4A89DC), 0);
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
    dstEnabled = lv_obj_get_state(obj); // oder lv_obj_get_state(obj) & LV_STATE_CHECKED für lvgl v7
    saveDSTEnabled(); // Speichere den neuen Zustand

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

    // Erstellen und positionieren Sie die Überschrift
    lv_obj_t * label = lv_label_create(datetimeContainer);
    lv_label_set_text(label, "Datum und Uhrzeit einstellen");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);  // Positionieren Sie die Überschrift oben im Container

    // Erstellen Sie Roller für Tag, Monat, Jahr, Stunde und Minute
    dayRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(dayRoller, "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(dayRoller, LV_ALIGN_CENTER, -130, -50);

    monthRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(monthRoller, "Jan\nFeb\nMar\nApr\nMay\nJun\nJul\nAug\nSep\nOct\nNov\nDec", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(monthRoller, LV_ALIGN_CENTER, -70, -50);

    yearRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(yearRoller, "2020\n2021\n2022\n2023\n2024\n2025\n2026\n2027\n2028\n2029\n2030", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(yearRoller, LV_ALIGN_CENTER, 0, -50);

    hourRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(hourRoller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(hourRoller, LV_ALIGN_CENTER, 70, -50);

    minuteRoller = lv_roller_create(datetimeContainer);
    lv_roller_set_options(minuteRoller, "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59", LV_ROLLER_MODE_INFINITE);
    lv_obj_align(minuteRoller, LV_ALIGN_CENTER, 130, -50);

    DateTime now = getRTCDateTime(); // Obtain the current DateTime from RTC
    setDateTimeRollersToCurrent(); // Pass the 'now' as argument

    lv_obj_t *dstSwitch = lv_switch_create(datetimeContainer);
    lv_obj_align(dstSwitch, LV_ALIGN_CENTER, 0, 70);
    lv_obj_add_event_cb(dstSwitch, dst_switch_event_cb, LV_EVENT_CLICKED, NULL);
    if (dstEnabled) {
        lv_obj_add_state(dstSwitch, LV_STATE_CHECKED); // Turn the switch "on"
    } else {
        lv_obj_clear_state(dstSwitch, LV_STATE_CHECKED); // Turn the switch "off"
    }
    // Erstellen und positionieren Sie die Überschrift
    lv_obj_t * label1 = lv_label_create(datetimeContainer);
    lv_label_set_text(label1, "Sommer-/Winterumschaltung");
    lv_obj_align(label1, LV_ALIGN_CENTER, 0, 50);  // Positionieren Sie die Überschrift oben im Container

    // Hinzufügen der Bestätigungs- und Abbrechen-Schaltflächen
    lv_obj_t * btnm = lv_btnmatrix_create(datetimeContainer);
    
    static const char * btnm_map[] = {"OK", "Abbrechen", ""};
    lv_btnmatrix_set_map(btnm, btnm_map);
    lv_obj_set_size(btnm, 240, 60); // Breite und Höhe der Buttonmatrix

    // Setze die Anzahl der Spalten und Zeilen für die Buttonmatrix
    // Hier nehmen wir an, dass Sie zwei Schaltflächen nebeneinander wollen, jede 120x60
    lv_btnmatrix_set_btn_ctrl_all(btnm, LV_BTNMATRIX_CTRL_CHECKABLE); // Nur wenn die Buttons "checkable" sein sollen
    lv_btnmatrix_set_btn_width(btnm, 0, 2); // Setzt die Breite des ersten Buttons auf '2' Einheiten
    lv_btnmatrix_set_btn_width(btnm, 1, 2); // Setzt die Breite des zweiten Buttons auf '2' Einheiten

    // Schließlich die Schaltflächen im Container zentrieren
    lv_obj_align(btnm, LV_ALIGN_CENTER, 0, 150); // Zentriert die Buttonmatrix im Container
    // Hinzufügen von Event-Callbacks für die Schaltflächen
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
