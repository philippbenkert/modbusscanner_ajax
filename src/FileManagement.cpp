#include "FileManagement.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"
#include "Recipe.h"

const uint32_t debounce_period_ms = 500; // Debounce period in milliseconds
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
lv_obj_t* chart = nullptr; // or some initial value
lv_chart_series_t* ser = nullptr; // or some initial value

bool is_update_timer_active = false;
bool dropdown_exists = false; // Globale Variable am Anfang des Codes
// Globale Variable für das Dropdown-Objekt
lv_obj_t* recipe_dropdown = nullptr;
int selectedRecipeIndex = 0;
bool pendingUpdate = false;

//void updateChartBasedOnRecipe(const Recipe& recipe);
//void updateChartLabels(const Recipe& recipe, lv_obj_t* chart);
//void clearLabels(std::vector<lv_obj_t*>& labels);

// Event-Handler für die Rezeptauswahl
void recipe_dropdown_event_handler(lv_event_t * e) {
    Serial.println("Dropdown event handler called");

    uint32_t current_time = lv_tick_get();
    static uint32_t last_update_time = 0;

    if (current_time - last_update_time < debounce_period_ms) {
        Serial.println("Debounce active, returning");
        return; // Debounce: ignore if events happen too quickly
    }
    last_update_time = current_time;

    recipe_dropdown = lv_event_get_target(e);
    int newSelectedIndex = lv_dropdown_get_selected(recipe_dropdown);
    Serial.print("New selected index: ");
    Serial.println(newSelectedIndex);

    if (newSelectedIndex != selectedRecipeIndex) {
        selectedRecipeIndex = newSelectedIndex;
        pendingUpdate = true; // Update needed
        Serial.println("Update needed, setting pendingUpdate to true");

        // Direkte Aktualisierung des Charts
        if (lv_obj_is_valid(chart)) {
            updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
            pendingUpdate = false; // Update completed
        } else if (!is_update_timer_active && update_timer) {
            lv_timer_reset(update_timer); // Start the update timer if chart is not ready
            is_update_timer_active = true;
        }
    }
}

// Funktion zum Erstellen der Rezeptauswahl Dropdown-Liste
void createRecipeDropdown(lv_obj_t * parent) {
    // Erstellen eines neuen Dropdown-Menüs
    recipe_dropdown = lv_dropdown_create(parent);
    lv_dropdown_clear_options(recipe_dropdown);

    for (size_t i = 0; i < recipes.size(); ++i) {
        lv_dropdown_add_option(recipe_dropdown, recipes[i].name.c_str(), i);
        lv_dropdown_set_selected(recipe_dropdown, selectedRecipeIndex);
    }
    lv_obj_align(recipe_dropdown, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(recipe_dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    //dropdown_exists = true; // Setzen Sie die Flagge, dass das Dropdown jetzt existiert
}

// Dieser Handler wird aufgerufen, sobald das Layout des Charts aktualisiert wurde.
void chart_layout_updated_handler(lv_event_t * e) {
    static Recipe* last_recipe = nullptr;
    chart = lv_event_get_target(e);
    Recipe& current_recipe = recipes[selectedRecipeIndex];

    // Only update if the recipe has changed
    if (last_recipe != &current_recipe) {
        updateChartBasedOnRecipe(current_recipe);
        last_recipe = &current_recipe;
    }
}

void chart_ready_handler(lv_event_t * e) {
    chart = lv_event_get_target(e);
    const Recipe& recipe = recipes[selectedRecipeIndex];
    updateChartBasedOnRecipe(recipe);
}

// Wrapper function for the timer callback
void updateChartTimerCallback(lv_timer_t * timer) {
    Serial.println("Timer callback called");

     // Frühzeitiger Abbruch, wenn kein Update ansteht
    if (!pendingUpdate || !lv_obj_is_valid(chart)) {
        Serial.println("No pending update or chart is invalid, deactivating timer");
        is_update_timer_active = false;
        return;
    }

    Serial.println("Pending update detected");

    // Update the chart
    updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
    pendingUpdate = false; // Update done
    is_update_timer_active = false; // Timer deaktiviert
}

void fileManagementFunction(lv_event_t *e) {
    // Called when file management is invoked; initialize chart and dropdown
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        
        // Ensure the chart is created only once and is valid
        if (chart == nullptr || !lv_obj_is_valid(chart)) {
            Serial.println("Erstelle neues Chart");            
            chart = lv_chart_create(content_container);
            lv_obj_set_size(chart, TFT_WIDTH - 20, 150);
            lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -20);
            lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, MAX_DAYS);
            lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -24, 20);
            lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
            
            // Add series to the chart
            ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
            if (!ser) {
                Serial.println("Failed to add series to chart");
                return; // Exit if series cannot be added
            }
        }

        // Ensure the dropdown is created only once
        if (!dropdown_exists) {
            createRecipeDropdown(content_container);
            dropdown_exists = true;
        }

        if (!recipes.empty()) {
        Serial.print("Aktualisiere Chart mit Rezept: ");
        Serial.println(recipes[selectedRecipeIndex].name.c_str());
        updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
        } else {
        Serial.println("Keine Rezepte zum Anzeigen");
        }

        // Create or reset the update timer
        if (pendingUpdate) {
        if (update_timer) {
            lv_timer_reset(update_timer);
            is_update_timer_active = true;
        } else {
            update_timer = lv_timer_create(updateChartTimerCallback, 100, &recipes[selectedRecipeIndex]);
            is_update_timer_active = true;
        }
        }

        }
}
