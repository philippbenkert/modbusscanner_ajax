#include "FileManagement.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"
#include "Recipe.h"
#include <Preferences.h>
#include "DateTimeHandler.h"
#include "WLANSettings.h"
#include <esp_heap_caps.h>

extern SDCardHandler sdCard;
extern RTC_DS3231 rtc;
extern Preferences preferences;
std::vector<Recipe> recipes;
lv_obj_t* chart = nullptr;
lv_chart_series_t* ser = nullptr;
lv_obj_t* recipe_dropdown = nullptr;
lv_chart_series_t* progress_ser = nullptr; // Datenreihe für den Fortschritt

lv_obj_t* toggle_btn = nullptr;
lv_obj_t* label = nullptr;
lv_obj_t* label1 = nullptr;
lv_obj_t* btn = nullptr;
//lv_obj_t* end_time_label = nullptr;
lv_obj_t* save_btn = nullptr;
static lv_obj_t* end_time_label = nullptr;
DateTime now;
unsigned long startTime;
unsigned long startCoolingTime;
static lv_style_t save_btn_style;
static bool is_save_btn_style_initialized = false;
bool dropdown_exists = false;
int selectedRecipeIndex = 0;
int globalEndTime;
bool coolingProcessRunning;
void updateToggleCoolingButtonText();
#define BUF_SIZE 4096
byte buf[BUF_SIZE];
struct dblog_write_context wctx;
unsigned long savedEndTime = 0;
char buffer[30];



void loadCoolingProcessStatus() {
    preferences.begin("process", true);
    int test = preferences.getInt("coolingProcess", 0);
    if (test == 1) {
        coolingProcessRunning = true;
    } else {
        coolingProcessRunning = false;
    }
        savedEndTime = preferences.getULong("endTime", 0);
        startCoolingTime = preferences.getULong("starttime1", 0);
    preferences.end();
}

int callback(void *data, int argc, char **argv, char **azColName) {
    unsigned long *endTime = reinterpret_cast<unsigned long*>(data);
    if (argc > 0 && argv[0] != nullptr) {
        *endTime = strtoul(argv[0], nullptr, 10);
    }
    return 0;
}

unsigned long calculateEndTimeFromDb(const char* dbName) {
    // Die Funktion gibt einfach den Wert der globalen Variable zurück
    return globalEndTime;
}

void displayEndTime(unsigned long endTime) {
    // Prüfen, ob das Label noch gültig ist
    if (end_time_label && !lv_obj_is_valid(end_time_label)) {
        end_time_label = nullptr; // Setze die Referenz auf null, wenn das Label ungültig ist
    }

    // Einmalige Erstellung des Labels, falls es noch nicht existiert
    if (!end_time_label) {
        end_time_label = lv_label_create(content_container);
        lv_obj_align(end_time_label, LV_ALIGN_OUT_BOTTOM_MID, 20, 55);
    }

    // Textaktualisierung
    char buffer[30];
    if (endTime > 0 && coolingProcessRunning) {
        time_t endTimeSec = endTime;
        struct tm *endTimeStruct = localtime(&endTimeSec);
        if (strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", endTimeStruct) == 0) {
            lv_label_set_text(end_time_label, "Fehler bei der Zeitformatierung");
        } else {
            lv_label_set_text(end_time_label, buffer);
        }
    } else {
        lv_label_set_text(end_time_label, "Nicht gestartet");
    }
}

void printFreeHeap() {
    unsigned int freeHeap = esp_get_free_heap_size();
    Serial.print("Freier Heap-Speicher: ");
    Serial.print(freeHeap);
    Serial.println(" Bytes");
}

int flush_fn(struct dblog_write_context *ctx) {
    // Hier könnte eine Flush-Operation implementiert werden, falls nötig.
    // Für viele SD-Karten-Implementierungen könnte dies leer bleiben.
    return 0;
}

void startCoolingProcess() {
    if (selectedRecipeIndex < 0 || selectedRecipeIndex >= recipes.size()) {
        return;
    }
    coolingProcessRunning = true;
    updateToggleCoolingButtonText();
    const Recipe& selectedRecipe = recipes[selectedRecipeIndex];
    now = rtc.now();
    startTime = now.unixtime();
    char dbName[64];
    snprintf(dbName, sizeof(dbName), "/setpoint_%lu.db", startTime);
    sdCard.setDbPath(dbName);
    // Datenbank öffnen und initialisieren
    wctx.buf = buf;
    wctx.col_count = 2; // Anzahl der Spalten (Zeit und Temperatur)
    wctx.page_resv_bytes = 0;
    wctx.page_size_exp = 12; // Seitengröße als Exponent von 2 (hier 4096)
    wctx.read_fn = SDCardHandler::readData;
    wctx.write_fn = SDCardHandler::writeData;
    wctx.flush_fn = flush_fn;
    if (dblog_write_init(&wctx) != DBLOG_RES_OK) {
        return;
    }
    int stepsPerDay = 12;
    int stepDurationInSeconds = 2 * 60 * 60; // 2 Stunden pro Schritt in Sekunden
    for (size_t day = 0; day < selectedRecipe.temperatures.size() - 1; day++) {
        float startTemp = selectedRecipe.temperatures[day];
        float endTemp = selectedRecipe.temperatures[day + 1];
        float tempStep = (endTemp - startTemp) / stepsPerDay;
        for (int step = 0; step < stepsPerDay; step++) {
            float tempValue = std::round((startTemp + step * tempStep) * 10) / 10.0;
            unsigned long stepTime = startTime + day * 24 * 60 * 60 + step * stepDurationInSeconds;
            if (dblog_append_empty_row(&wctx) != DBLOG_RES_OK) {
                Serial.println("Fehler beim Einfügen einer leeren Reihe in die Datenbank");
                break;
            }
            if (dblog_set_col_val(&wctx, 0, DBLOG_TYPE_INT, &stepTime, sizeof(stepTime)) != DBLOG_RES_OK) {
                Serial.println("Fehler beim Einfügen der Zeit in die Datenbank");
                break;
            }
            if (dblog_set_col_val(&wctx, 1, DBLOG_TYPE_REAL, &tempValue, sizeof(tempValue)) != DBLOG_RES_OK) {
                Serial.println("Fehler beim Einfügen der Temperatur in die Datenbank");
                break;
            }
        }
    }
    // Datenbank abschließen
    if (dblog_finalize(&wctx) != DBLOG_RES_OK) {
        Serial.println("Fehler beim Finalisieren der Datenbank.");
    }
    // Endzeit berechnen und anzeigen
    unsigned long processDuration = (selectedRecipe.temperatures.size() - 1) * 24 * 60 * 60; // Gesamtdauer in Sekunden
    unsigned long endTime = startTime + processDuration;
    displayEndTime(endTime);
    // Status in den Festspeicher speichern
    preferences.begin("process", false);
    preferences.putInt("coolingProcess", 1);
    preferences.putULong("endTime", endTime);
    preferences.putString("currentDb", dbName);
    preferences.putULong("starttime1", startTime);
    preferences.end();
}

void stopCoolingProcess() {
    coolingProcessRunning = false;
    preferences.begin("process", true);
    String currentDb = preferences.getString("currentDb", "");
    preferences.end();
    preferences.begin("process", false);
    preferences.putInt("coolingProcess", 0);
    preferences.remove("currentDb");
    preferences.end();
    updateToggleCoolingButtonText();
    unsigned long endTime = calculateEndTimeFromDb(currentDb.c_str());
    displayEndTime(endTime);
}

static void confirm_cooling_stop_cb(lv_event_t * e) {
    lv_obj_t* mbox = reinterpret_cast<lv_obj_t*>(lv_event_get_user_data(e));
    const char* btn_text = lv_msgbox_get_active_btn_text(mbox);
    if (strcmp(btn_text, "OK") == 0) {
        // "OK" wurde gedrückt, stoppe den Kühlprozess
        stopCoolingProcess();
    }
    // Schließe das Popup in beiden Fällen
    lv_msgbox_close(mbox);
}

void updateToggleCoolingButtonText() {
    if (!toggle_btn || !lv_obj_is_valid(toggle_btn)) {
        return;
    }
    label = reinterpret_cast<lv_obj_t*>(lv_obj_get_user_data(toggle_btn));
    if (!label || !lv_obj_is_valid(label)) {
        return;
    }
    // Setzen des Button-Textes basierend auf dem Kühlprozessstatus
    lv_label_set_text(label, coolingProcessRunning ? "Stoppen" : "Starten");
}

void toggle_cooling_btn_event_cb(lv_event_t * e) {
    btn = lv_event_get_target(e);
    label = reinterpret_cast<lv_obj_t*>(lv_obj_get_user_data(btn));
    if (!coolingProcessRunning) {
        startCoolingProcess();
        updateToggleCoolingButtonText();
        coolingProcessRunning = true;
    } else {
        static const char* btns[] = {"OK", "Abbrechen", ""}; // Button-Beschriftungen
        lv_obj_t* mbox = lv_msgbox_create(lv_scr_act(), "Abbrechen bestaetigen", "Abkuehlprozess wirklich abbrechen?", btns, true);
        lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_event_cb(mbox, confirm_cooling_stop_cb, LV_EVENT_VALUE_CHANGED, mbox);
    }
}

void createToggleCoolingButton(lv_obj_t * parent) {
    toggle_btn = lv_btn_create(parent);
    lv_obj_align(toggle_btn, LV_ALIGN_OUT_BOTTOM_MID, 170, 47); // Positionierung
    lv_obj_add_event_cb(toggle_btn, toggle_cooling_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(toggle_btn, &save_btn_style, 0); // Stil auf den Button anwenden
    lv_obj_set_size(toggle_btn, 100, 30); // Größe des Buttons anpassen
    label = lv_label_create(toggle_btn);
    loadCoolingProcessStatus();
    lv_obj_center(label);
    lv_obj_set_user_data(toggle_btn, label); // Speichern Sie das Label im User-Daten des Buttons
}

void initialize_save_btn_style() {
    if (!is_save_btn_style_initialized) {
        lv_style_init(&save_btn_style);
        lv_style_set_bg_color(&save_btn_style, lv_color_hex(0x00AEEF));
        lv_style_set_bg_opa(&save_btn_style, LV_OPA_COVER);
        is_save_btn_style_initialized = true;
    }
}

void showSaveConfirmationPopup() {
    lv_obj_t* mbox = lv_msgbox_create(lv_scr_act(), "Gespeichert", "Das Rezept wurde erfolgreich gespeichert!", nullptr, true);
    lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, LV_PART_MAIN);
    // Setze einen Timer, um das Popup automatisch zu schließen
    lv_timer_t* timer = lv_timer_create([](lv_timer_t * timer) {
        lv_obj_t* mbox = reinterpret_cast<lv_obj_t*>(timer->user_data);
            if(lv_obj_is_valid(mbox)) { // Überprüfen Sie die Gültigkeit des Objekts
            lv_msgbox_close(mbox);
            }
            lv_timer_del(timer);
    }, 2000, mbox); // Schließt das Popup nach 2 Sekunden
}

void saveSelectedRecipe() {
    if (recipes.empty() || selectedRecipeIndex < 0 || selectedRecipeIndex >= recipes.size()) {
        return;
    }
    preferences.begin("recipe_app", false); // "false" für schreibzugriff
    preferences.putInt("selectedRecipe", selectedRecipeIndex);
    preferences.end();
}

static void save_button_event_handler(lv_event_t* e) {
    if (coolingProcessRunning) {
        // Der Button ist deaktiviert; führe keine Aktion aus
        return;
    }
    saveSelectedRecipe();
    showSaveConfirmationPopup();
}

void createSaveButton(lv_obj_t * parent) {
    if (!parent || !lv_obj_is_valid(parent)) return;
    initialize_save_btn_style();
    save_btn = lv_btn_create(parent);
    lv_obj_add_style(save_btn, &save_btn_style, 0); // Stil auf den Button anwenden
    lv_obj_set_size(save_btn, 100, 30); // Größe des Buttons anpassen
    lv_obj_align(save_btn, LV_ALIGN_OUT_BOTTOM_MID, 170, 10); // Position unterhalb des Dropdown-Menüs
    label1 = lv_label_create(save_btn);
    lv_label_set_text(label1, "Speichern");
    // Event-Callback für den Button hinzufügen
    lv_obj_add_event_cb(save_btn, save_button_event_handler, LV_EVENT_CLICKED, NULL);
}

void updateSaveButtonState() {
    if (save_btn && lv_obj_is_valid(save_btn)) {
        if (coolingProcessRunning == true) {
            lv_obj_add_state(save_btn, LV_STATE_DISABLED);
            lv_obj_remove_event_cb(save_btn, save_button_event_handler); // Event-Callback entfernen
        } else {
            lv_obj_clear_state(save_btn, LV_STATE_DISABLED);
            lv_obj_add_event_cb(save_btn, save_button_event_handler, LV_EVENT_CLICKED, NULL); // Event-Callback hinzufügen
        }
    }
}

void recipe_dropdown_event_handler(lv_event_t * e) {
    if (coolingProcessRunning) {
        // Der Button ist deaktiviert; führe keine Aktion aus
        return;
    }
    recipe_dropdown = lv_event_get_target(e);
    if (!recipe_dropdown || !lv_obj_is_valid(recipe_dropdown)) return;
    int newSelectedIndex = lv_dropdown_get_selected(recipe_dropdown);
    if (newSelectedIndex != selectedRecipeIndex && !recipes.empty()) {
        selectedRecipeIndex = newSelectedIndex;
        if (chart && lv_obj_is_valid(chart)) {
            updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
        }
    }
}

void createRecipeDropdown(lv_obj_t *parent)
{
    if (!parent || !lv_obj_is_valid(parent))
        return;
    if (!dropdown_exists)
    {
        recipe_dropdown = lv_dropdown_create(parent);
        lv_dropdown_clear_options(recipe_dropdown);
        for (size_t i = 0; i < recipes.size(); ++i)
        {
            lv_dropdown_add_option(recipe_dropdown, recipes[i].name.c_str(), i);
        }
        lv_dropdown_set_selected(recipe_dropdown, selectedRecipeIndex);
        lv_obj_set_size(recipe_dropdown, 150, 30); // Größe des Buttons anpassen
        lv_obj_align(recipe_dropdown, LV_ALIGN_TOP_MID, -50, 10);
        lv_obj_add_event_cb(recipe_dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_style(recipe_dropdown, &style_no_border, 0);
        dropdown_exists = true;
    }
}

void updateRecipeDropdownState() {
    if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
        if (coolingProcessRunning == true) {
            lv_obj_add_state(recipe_dropdown, LV_STATE_DISABLED);
            lv_obj_remove_event_cb(save_btn, recipe_dropdown_event_handler); // Event-Callback entfernen
        } else {
            lv_obj_clear_state(recipe_dropdown, LV_STATE_DISABLED);
            lv_obj_add_event_cb(save_btn, recipe_dropdown_event_handler, LV_EVENT_CLICKED, NULL); // Event-Callback hinzufügen
        }
    }
}

void fileManagementFunction(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    preferences.begin("recipe_app", true); // "true" für lesezugriff
    int storedIndex = preferences.getInt("selectedRecipe", -1); // Standardwert auf -1 setzen
    preferences.end();
    if (storedIndex >= 0 && storedIndex < recipes.size()) {
        selectedRecipeIndex = storedIndex;
    }
    if (!recipes.empty()) {
        updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
        isMenuLocked = false;
    }
    loadCoolingProcessStatus();
    updateToggleCoolingButtonText();
}