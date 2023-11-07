#include "FileManagement.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"
#include "Recipe.h"


const int MAX_DAYS = 14;
const int Y_AXIS_PADDING = 2;
const int X_LABEL_OFFSET = 35;
const int Y_LABEL_OFFSET = 130;
const int TEMP_LABEL_Y_OFFSET = 100;
// Global variable
lv_timer_t* update_timer = nullptr;
std::vector<lv_obj_t*> x_axis_labels;
std::vector<Recipe> recipes;
std::vector<lv_obj_t*> temp_labels;

bool is_update_timer_active = false;
bool dropdown_exists = false; // Globale Variable am Anfang des Codes
// Globale Variable für das Dropdown-Objekt
lv_obj_t* recipe_dropdown = nullptr;
int selectedRecipeIndex = 0;
bool pendingUpdate = false;

void updateChartBasedOnRecipe(const Recipe& recipe);
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart);
void clearLabels(std::vector<lv_obj_t*>& labels);

// Event-Handler für die Rezeptauswahl
void recipe_dropdown_event_handler(lv_event_t * e) {
    static uint32_t last_update_time = 0;
    const uint32_t debounce_period_ms = 500; // 500 Millisekunden Entprellzeit

    uint32_t current_time = lv_tick_get();
    if (current_time - last_update_time < debounce_period_ms) {
        return; // Entprellung: Ignoriere Ereignisse, die zu schnell aufeinander folgen
    }
    last_update_time = current_time;

    lv_obj_t * dropdown = lv_event_get_target(e);
    int newSelectedIndex = lv_dropdown_get_selected(dropdown);
    if (newSelectedIndex != selectedRecipeIndex) {
        selectedRecipeIndex = newSelectedIndex;
        pendingUpdate = true; // Ein Update steht aus
        // Starte den Timer nur, wenn kein Update aussteht und der Timer nicht aktiv ist
        if (!is_update_timer_active && !pendingUpdate) {
            is_update_timer_active = true; // Setzen Sie den Timer-Status auf aktiv
            lv_timer_reset(update_timer); // Setze den Timer zurück, um die Aktualisierung sofort zu starten
        }
    }
}


void deleteRecipeDropdown() {
    if (recipe_dropdown) {
        lv_obj_del(recipe_dropdown); // Löscht das Dropdown-Objekt
        recipe_dropdown = nullptr; // Setzt den Zeiger auf nullptr
        dropdown_exists = false; // Setzt die Flagge zurück, dass kein Dropdown existiert
    }
}
// Funktion zum Erstellen der Rezeptauswahl Dropdown-Liste
void createRecipeDropdown(lv_obj_t * parent) {
    // Löschen des alten Dropdown-Menüs, falls vorhanden
    deleteRecipeDropdown();

    // Erstellen eines neuen Dropdown-Menüs
    recipe_dropdown = lv_dropdown_create(parent);
    lv_dropdown_clear_options(recipe_dropdown);

    for (size_t i = 0; i < recipes.size(); ++i) {
        lv_dropdown_add_option(recipe_dropdown, recipes[i].name.c_str(), i);
    }
    lv_obj_align(recipe_dropdown, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(recipe_dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    dropdown_exists = true; // Setzen Sie die Flagge, dass das Dropdown jetzt existiert
}

// Dieser Handler wird aufgerufen, sobald das Layout des Charts aktualisiert wurde.
void chart_layout_updated_handler(lv_event_t * e) {
    static Recipe* last_recipe = nullptr;
    lv_obj_t * chart = lv_event_get_target(e);
    Recipe& current_recipe = recipes[selectedRecipeIndex];

    // Only update if the recipe has changed
    if (last_recipe != &current_recipe) {
        updateChartBasedOnRecipe(current_recipe);
        last_recipe = &current_recipe;
    }
}

void chart_ready_handler(lv_event_t * e) {
    lv_obj_t * chart = lv_event_get_target(e);
    const Recipe& recipe = recipes[selectedRecipeIndex];
    updateChartBasedOnRecipe(recipe);
}

// Wrapper function for the timer callback
void updateChartTimerCallback(lv_timer_t * timer) {
    if (pendingUpdate && !is_update_timer_active) {
        Recipe* recipe = static_cast<Recipe*>(timer->user_data);
        if (recipe && lv_obj_is_valid(chart) && recipe == &recipes[selectedRecipeIndex]) {
            updateChartBasedOnRecipe(*recipe);
            pendingUpdate = false; // Das Update wurde durchgeführt
            is_update_timer_active = false; // Timer-Status aktualisieren, da die Aktualisierung abgeschlossen ist
        }
    }
}

void fileManagementFunction(lv_event_t *e) {
    Serial.println("fileManagementFunction aufgerufen"); // Debugging-Ausgabe

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        Serial.println("Event ist nicht LV_EVENT_CLICKED, Funktion wird verlassen"); // Debugging-Ausgabe
        return;
    }

    // Stoppen und Löschen des alten Timers, falls vorhanden
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = nullptr;
        is_update_timer_active = false; // Setzen Sie den Status auf inaktiv
    }

    // Löschen des alten Charts und seiner Kinder, falls vorhanden
    if (chart && lv_obj_is_valid(chart)) {
        Serial.println("Lösche vorhandenes Chart und seine Kinder");
        lv_obj_clean(chart); // Entfernt alle Kinder des Charts
        lv_obj_del(chart);
        chart = nullptr; // Setze den Zeiger auf nullptr nach dem Löschen
    } else {
        Serial.println("Kein Chart zu löschen oder Chart ist ungültig");
    }

    // Bereinigen Sie den Inhalt des Containers, bevor Sie neue Elemente hinzufügen
    clearContentArea();

    // Erstellung des neuen Charts
    if (chart == nullptr || !lv_obj_is_valid(chart)) {
        Serial.println("Erstelle neues Chart, da kein gültiges Chart vorhanden ist.");
        // Erstellung des neuen Charts
        chart = lv_chart_create(content_container);
        if (!chart) {
            Serial.println("Chart konnte nicht erstellt werden"); // Debugging-Ausgabe
            return; // Frühzeitiger Ausstieg, wenn die Erstellung fehlschlägt
        }
        lv_obj_set_size(chart, TFT_WIDTH - 20, 150);
        lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, MAX_DAYS);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -24, 20);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
        if (!ser) {
            Serial.println("Failed to add series to chart");
            return;
        }
    } else {
        Serial.println("Chart ist bereits vorhanden und gültig.");
    }
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    if (!ser) {
        Serial.println("Failed to add series to chart");
        return;
    }

    deleteRecipeDropdown();

    // Create the dropdown list for recipe selection
    if (!dropdown_exists) {
        createRecipeDropdown(content_container);
        dropdown_exists = true; // Setzen Sie eine Flagge, dass das Dropdown jetzt existiert
    }
    // Register the event callbacks for the new chart
    if (chart && lv_obj_is_valid(chart)) {
    Serial.println("Lösche vorhandenes Chart und seine Kinder");
    //lv_obj_clean(chart); // Entfernt alle Kinder des Charts
    //lv_obj_del(chart); // Löscht das Chart-Objekt
    //chart = nullptr; // Setze den Zeiger auf nullptr nach dem Löschen
} else {
    Serial.println("Kein Chart zu löschen oder Chart ist ungültig");
}
    // Initialize the chart with the first recipe if the recipe list is not empty
    if (!recipes.empty()) {
        Serial.println("Initialisiere Chart mit erstem Rezept"); // Debugging-Ausgabe
        if (!update_timer) {
        update_timer = lv_timer_create(updateChartTimerCallback, 100, &recipes[selectedRecipeIndex]);
        if (update_timer) {
            Serial.println("Timer erstellt und bereit");
            is_update_timer_active = true; // Setze den Status auf aktiv
        } else {
            Serial.println("Timer konnte nicht erstellt werden");
        }
    }
}
}

