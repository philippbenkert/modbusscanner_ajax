#include "FileManagement.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"

const int MAX_DAYS = 14;
const int Y_AXIS_PADDING = 2;
const int X_LABEL_OFFSET = 35;
const int Y_LABEL_OFFSET = 130;
const int TEMP_LABEL_Y_OFFSET = 100;

struct Recipe {
    String name;
    std::vector<int> temperatures;
};

std::vector<lv_obj_t*> x_axis_labels;
std::vector<Recipe> recipes;
std::vector<lv_obj_t*> temp_labels;

int selectedRecipeIndex = 0;

void updateChartBasedOnRecipe(const Recipe& recipe);
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart);

void clearLabels(std::vector<lv_obj_t*>& labels) {
    for (auto label : labels) {
        lv_obj_del(label);
    }
    labels.clear();
}

void clearXAxisLabels() {
    for (auto label : x_axis_labels) {
        lv_obj_del(label); // Delete the label object
    }
    x_axis_labels.clear(); // Clear the vector for new labels
}

void clearTempLabels() {
    for (auto label : temp_labels) {
        lv_obj_del(label);
    }
    temp_labels.clear();
}

lv_obj_t* createLabel(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

// Update the chart labels to be children of the chart container
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart) {
    // Clear existing labels first
    clearLabels(x_axis_labels);
    clearLabels(temp_labels);

    const lv_coord_t chart_width = lv_obj_get_width(chart);
    const lv_coord_t chart_height = lv_obj_get_height(chart);
    const lv_coord_t chart_x = lv_obj_get_x(chart);
    const lv_coord_t chart_y = lv_obj_get_y(chart);
    
    // Create x-axis labels as children of the chart container
    for (size_t i = 0; i < recipe.temperatures.size(); ++i) {
        char buf[10];
        snprintf(buf, sizeof(buf), "%zu", i);
        int x_offset = chart_x + (i * chart_width) / recipe.temperatures.size() + X_LABEL_OFFSET;
        int y_offset = chart_y + chart_height + Y_LABEL_OFFSET;
        x_axis_labels.push_back(createLabel(chart, buf, x_offset, y_offset));
    }

    // Create temperature labels as children of the chart container
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    float y_scale = (float)chart_height / (max_temp - min_temp);
    for (size_t i = 0; i < recipe.temperatures.size(); ++i) {
        char buf[10];
        snprintf(buf, sizeof(buf), "%d", recipe.temperatures[i]);
        int x_pos = chart_x + (i * chart_width) / recipe.temperatures.size() + X_LABEL_OFFSET;
        int y_pos = chart_y + chart_height - ((recipe.temperatures[i] - min_temp) * y_scale) + TEMP_LABEL_Y_OFFSET;
        temp_labels.push_back(createLabel(chart, buf, x_pos, y_pos));
    }
}

// Funktion zum Einlesen der CSV-Datei
void readRecipesFromFile() {
    File file = LittleFS.open("/kurven.csv", "r");
    if (!file) {
        return;
    }
    file.readStringUntil('\n');
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim(); // Entfernt Whitespace und Zeilenumbrüche
        if (line.length() == 0) {
            continue;
        }
        int delimiterPos = 0;
        Recipe recipe;
        recipe.temperatures.reserve(MAX_DAYS); // MAX_DAYS sollte die erwartete Anzahl von Tagen sein
        delimiterPos = line.indexOf(';');
        recipe.name = line.substring(0, delimiterPos);
        line = line.substring(delimiterPos + 1); // Entferne den Rezeptnamen und das Semikolon
        // Lese die Temperaturen
        while ((delimiterPos = line.indexOf(';')) != -1) {
            String tempStr = line.substring(0, delimiterPos);
            line = line.substring(delimiterPos + 1);
            recipe.temperatures.push_back(tempStr.toInt());
        }
        // Füge die letzte Temperatur hinzu (nach dem letzten Semikolon)
        recipe.temperatures.push_back(line.toInt());
        // Füge das Rezept zum Vektor hinzu
        recipes.push_back(recipe);
    }
    file.close();
}

// Event-Handler für die Rezeptauswahl
void recipe_dropdown_event_handler(lv_event_t * e) {
    lv_obj_t * dropdown = lv_event_get_target(e);
    selectedRecipeIndex = lv_dropdown_get_selected(dropdown);
    updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
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
    lv_obj_t * chart = lv_event_get_target(e);
    updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
}

// Funktion zum Aktualisieren des Charts basierend auf einem Rezept
void updateChartBasedOnRecipe(const Recipe& recipe) {
    if (!chart || !ser) {
        return;
    }

    clearXAxisLabels();
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, recipe.temperatures.size());
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
    lv_chart_remove_series(chart, ser);
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_point_count(chart, recipe.temperatures.size());

    // Füge die Temperaturwerte der Serie hinzu
    for (size_t i = 0; i < recipe.temperatures.size(); i++) {
        lv_chart_set_next_value(chart, ser, recipe.temperatures[i]);
    }
    updateChartLabels(recipe, chart); // Korrigierter Aufruf mit drei Parametern
}

void chart_ready_handler(lv_event_t * e) {
    lv_obj_t * chart = lv_event_get_target(e);
    const Recipe& recipe = recipes[selectedRecipeIndex];
    updateChartBasedOnRecipe(recipe);
}

// Wrapper function for the timer callback
void updateChartTimerCallback(lv_timer_t * timer) {
    // Zugriff auf user_data direkt aus der lv_timer_t Struktur
    Recipe* recipe = static_cast<Recipe*>(timer->user_data);
    if(recipe != nullptr) {
        updateChartBasedOnRecipe(*recipe);
    }
}

void fileManagementFunction(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    clearContentArea();
    // Chart-Erstellung
    chart = lv_chart_create(content_container);
    if (!chart) {
        return;
    }
    lv_obj_set_size(chart, TFT_WIDTH - 20, 150);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, MAX_DAYS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -24, 20);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    if (!ser) {
        return;
    }
    // Erstelle die Rezeptauswahl Dropdown-Liste
    createRecipeDropdown(content_container);
    lv_obj_invalidate(chart); // Tell LittlevGL that this object needs to be redrawn
    lv_task_handler(); // Manually call the task handler to ensure the chart is rendered
    // Registriere den Layout-Update-Handler für das Chart
    lv_obj_add_event_cb(chart, chart_layout_updated_handler, LV_EVENT_LAYOUT_CHANGED, NULL);
    lv_obj_add_event_cb(chart, chart_ready_handler, LV_EVENT_READY, NULL);
    // Initialisiere das Chart mit dem ersten Rezept
    if (!recipes.empty()) {
        lv_timer_t* timer = lv_timer_create(updateChartTimerCallback, 100, &recipes[0]);
        lv_timer_ready(timer);
    }
}