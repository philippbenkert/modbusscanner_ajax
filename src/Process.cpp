#include "Process.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"
#include "Recipe.h"
#include <Preferences.h>
#include "DateTimeHandler.h"
#include "WLANSettings.h"
#include <esp_heap_caps.h>
#include <memory>
#include "DatabaseHandler.h"

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
lv_obj_t* btn = nullptr;
lv_obj_t* end_time_label = nullptr;
//static lv_obj_t* end_time_label = nullptr;
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
//struct dblog_write_context wctx;
unsigned long savedEndTime = 0;
char buffer[30];
char dbName[64];


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
        end_time_label = nullptr;
    }

    // Einmalige Erstellung des Labels, falls es noch nicht existiert
    if (!end_time_label) {
        end_time_label = lv_label_create(content_container);
        lv_obj_align(end_time_label, LV_ALIGN_OUT_BOTTOM_MID, 10, 55);
    }

    char buffer[64];
    if (endTime > 0 && coolingProcessRunning) {
        time_t endTimeSec = endTime;
        struct tm *endTimeStruct = localtime(&endTimeSec);
        if (strftime(buffer, sizeof(buffer), "Vorgang endet am %d.%m.%Y %H:%M", endTimeStruct) == 0) {
            snprintf(buffer, sizeof(buffer), "Vorgang endet: Fehler bei der Zeitformatierung");
        }
    } else {
        snprintf(buffer, sizeof(buffer), "");
    }

    if (end_time_label && lv_obj_is_valid(end_time_label)) {
        lv_label_set_text(end_time_label, buffer);
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

    // Endzeit berechnen und anzeigen
    unsigned long processDuration = (selectedRecipe.temperatures.size() - 1) * 24 * 60 * 60;
    unsigned long endTime = startTime + processDuration;
    displayEndTime(endTime);

    // Status in den Festspeicher speichern
    preferences.begin("process", false);
    preferences.putInt("coolingProcess", 1);
    preferences.putULong("endTime", endTime);
    preferences.putULong("starttime1", startTime);
    preferences.end();
}

void stopCoolingProcess() {
    coolingProcessRunning = false;
    preferences.begin("process", true);
    preferences.end();
    preferences.begin("process", false);
    preferences.putInt("coolingProcess", 0);
    
    preferences.end();

    if (chart && progress_ser) {
                lv_chart_set_point_count(chart, lv_chart_get_point_count(chart));
                for (int i = 0; i < lv_chart_get_point_count(chart); i++) {
                    lv_chart_set_next_value(chart, progress_ser, LV_CHART_POINT_NONE);
                }
            }

    const char* dbName = "/sd/setpoints.db";
    std::string tableName = "Setpoints";

    exportDataToXML(dbName, tableName, startCoolingTime);
    sdCard.deleteRowsWithCondition("Setpoints", 1700000000);
    updateToggleCoolingButtonText();
}
    

static void confirm_cooling_stop_cb(lv_event_t * e) {
    lv_obj_t* mbox = reinterpret_cast<lv_obj_t*>(lv_event_get_user_data(e));
    const char* btn_text = lv_msgbox_get_active_btn_text(mbox);
    if (strcmp(btn_text, "OK") == 0) {
        // "OK" wurde gedrückt, stoppe den KühlProcess
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

    // Setzen des Button-Textes basierend auf dem KühlProcessstatus
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
        lv_obj_t* mbox = lv_msgbox_create(lv_scr_act(), "Abbrechen bestaetigen", "AbkuehlProcess wirklich abbrechen?", btns, true);
        lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_event_cb(mbox, confirm_cooling_stop_cb, LV_EVENT_VALUE_CHANGED, mbox);
    }
}

void createToggleCoolingButton(lv_obj_t * parent) {
    toggle_btn = lv_btn_create(parent);
    lv_obj_align(toggle_btn, LV_ALIGN_OUT_BOTTOM_MID, 175, 5); // Positionierung
    lv_obj_add_event_cb(toggle_btn, toggle_cooling_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(toggle_btn, &save_btn_style, 0); // Stil auf den Button anwenden
    lv_obj_set_size(toggle_btn, 100, 30); // Größe des Buttons anpassen
    label = lv_label_create(toggle_btn);
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

void saveSelectedRecipe() {
    if (recipes.empty() || selectedRecipeIndex < 0 || selectedRecipeIndex >= recipes.size()) {
        return;
    }
    preferences.begin("recipe_app", false); // "false" für schreibzugriff
    preferences.putInt("selectedRecipe", selectedRecipeIndex);
    preferences.end();
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
        snprintf(dbName, sizeof(dbName), "/setpoint.db", startTime);
        sdCard.setDbPath(dbName);

        // Datenbank öffnen und initialisieren
        if (!sdCard.openDatabase("/sd/setpoint.db")) {
            Serial.println("Failed to open database");
            return;
        }

        // Erstellen Sie eine neue Tabelle für die Setpoints des ausgewählten Rezepts
        std::string setpointTableName = "Setpoints";
        if (!sdCard.createSetpointTable(setpointTableName)) {
            Serial.println("Fehler beim Erstellen der Setpoint-Tabelle.");
            return;
        }

        // Bereiten Sie das Einfügen der Daten vor und starten Sie die Transaktion
        sdCard.prepareInsertStatement("Setpoints");
        sdCard.beginTransaction();

        if (!sdCard.prepareInsertStatement("Setpoints")) {
        Serial.println("Failed to prepare insert statement");
        return;
        }

        const Recipe& selectedRecipe = recipes[selectedRecipeIndex];
        int stepsPerDay = 4; // 24 Stunden / 6 Stunden pro Schritt = 4 Schritte pro Tag
        int stepDurationInSeconds = 6 * 60 * 60; // 6 Stunden in Sekunden
        for (size_t day = 0; day < selectedRecipe.temperatures.size(); day++) {
            float startTemp = selectedRecipe.temperatures[day];
            float endTemp = (day < selectedRecipe.temperatures.size() - 1) ? selectedRecipe.temperatures[day + 1] : startTemp;

            for (int step = 0; step < stepsPerDay; step++) {
                // Am letzten Tag nur einen Sollwert setzen (den Endwert des Rezepts)
                if (day == selectedRecipe.temperatures.size() - 1 && step > 0) break;

                float tempValue = (day < selectedRecipe.temperatures.size() - 1) ? 
                                  startTemp + ((endTemp - startTemp) * step / stepsPerDay) : 
                                  startTemp;

                unsigned long stepTime = startTime + day * 24 * 60 * 60 + step * stepDurationInSeconds;
                if (!sdCard.logSetpointData(stepTime, tempValue * 1000)) {
                    Serial.println("Error logging data to database");
                    break;
                }
            }
        }
        sdCard.endTransaction();

        if (chart && lv_obj_is_valid(chart)) {
            saveSelectedRecipe();
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
        lv_obj_align(recipe_dropdown, LV_ALIGN_TOP_MID, -45, 5);
        lv_obj_add_event_cb(recipe_dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_style(recipe_dropdown, &style_no_border, 0);
        dropdown_exists = true;
    }
}

void updateRecipeDropdownState() {
    if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
        if (coolingProcessRunning == true) {
            lv_obj_add_state(recipe_dropdown, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(recipe_dropdown, LV_STATE_DISABLED);
        }
    }
}

void ProcessFunction(lv_event_t *e) {
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
    updateToggleCoolingButtonText();
}