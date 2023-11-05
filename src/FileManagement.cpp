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

int selectedRecipeIndex = 0;

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
        if (update_timer) {
            // Aktualisiere den Timer nur, wenn sich das ausgewählte Rezept geändert hat
            lv_timer_set_repeat_count(update_timer, 1); // Stelle sicher, dass der Timer nur einmal ausgelöst wird
            lv_timer_reset(update_timer); // Setze den Timer zurück, um die Aktualisierung sofort zu starten
        }
        updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
    }
}

// Funktion zum Erstellen der Rezeptauswahl Dropdown-Liste
void createRecipeDropdown(lv_obj_t * parent) {
    lv_obj_t * dropdown = lv_dropdown_create(parent);
    lv_dropdown_clear_options(dropdown);

    for (size_t i = 0; i < recipes.size(); ++i) {
        lv_dropdown_add_option(dropdown, recipes[i].name.c_str(), i);
    }
    lv_obj_align(dropdown, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
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
    // Zugriff auf user_data direkt vom lv_timer_t-Struktur
    Recipe* recipe = static_cast<Recipe*>(timer->user_data);
    if(recipe != nullptr && recipe == &recipes[selectedRecipeIndex]) {
        updateChartBasedOnRecipe(*recipe);
    }
}

void fileManagementFunction(lv_event_t *e) {
    Serial.println("fileManagementFunction aufgerufen"); // Debugging-Ausgabe

    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        Serial.println("Event ist nicht LV_EVENT_CLICKED, Funktion wird verlassen"); // Debugging-Ausgabe
        return;
    }

    // Stoppen und Löschen des alten Timers, falls vorhanden
    if (update_timer && is_update_timer_active) {
        Serial.println("Lösche vorhandenen Timer");
        lv_timer_del(update_timer);
        update_timer = nullptr;
        is_update_timer_active = false; // Setze den Status auf inaktiv
    } else {
        Serial.println("Kein Timer zu löschen oder Timer bereits inaktiv");
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
    chart = lv_chart_create(content_container);
    if (!chart) {
        Serial.println("Chart konnte nicht erstellt werden"); // Debugging-Ausgabe
        return; // Frühzeitiger Ausstieg, wenn die Erstellung fehlschlägt
    }
    Serial.println("Neues Chart erstellt"); // Debugging-Ausgabe

    lv_obj_set_size(chart, TFT_WIDTH - 20, 150);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -20);
    if (lv_obj_check_type(chart, &lv_chart_class)) {
    Serial.println("Chart-Objekt ist gültig, setze Bereich"); // Debugging-Ausgabe
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, MAX_DAYS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -24, 20);
} else {
    Serial.println("Chart-Objekt ist NICHT gültig, Bereich wird NICHT gesetzt"); // Debugging-Ausgabe
    return;
}lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    if (!ser) {
        Serial.println("Failed to add series to chart");
        return;
    }

    // Create the dropdown list for recipe selection
    createRecipeDropdown(content_container);

    // Register the event callbacks for the new chart
    lv_obj_add_event_cb(chart, chart_layout_updated_handler, LV_EVENT_LAYOUT_CHANGED, NULL);
    lv_obj_add_event_cb(chart, chart_ready_handler, LV_EVENT_READY, NULL);

    // Initialize the chart with the first recipe if the recipe list is not empty
    if (!recipes.empty()) {
        Serial.println("Initialisiere Chart mit erstem Rezept"); // Debugging-Ausgabe
        update_timer = lv_timer_create(updateChartTimerCallback, 100, &recipes[selectedRecipeIndex]);
        if (update_timer) {
    Serial.println("Timer erstellt und bereit");
    is_update_timer_active = true; // Setze den Status auf aktiv
} else {
    Serial.println("Timer konnte nicht erstellt werden");

    } 

}
}

