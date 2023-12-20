#include "Process.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"
#include "Recipe.h"
#include <Preferences.h>
#include <esp_heap_caps.h>
#include <memory>
#include "ToggleButtons.h"

extern SDCardHandler sdCard;
extern Preferences preferences;
std::vector<Recipe> recipes;
lv_obj_t* chart = nullptr;
lv_chart_series_t* ser = nullptr;

lv_obj_t* end_time_label = nullptr;
unsigned long startTime;
lv_style_t save_btn_style;
static bool is_save_btn_style_initialized = false;
int selectedRecipeIndex = 0;
int globalEndTime;
bool coolingProcessRunning;
unsigned long savedEndTime = 0;

void loadCoolingProcessStatus() {
    preferences.begin("process", true);
    int test = preferences.getInt("coolingProcess", 0);
    if (test == 1) {
        coolingProcessRunning = true;
    } else {
        coolingProcessRunning = false;
    }
        savedEndTime = preferences.getULong("endTime", 0);
        startTime = preferences.getULong("starttime1", 0);
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