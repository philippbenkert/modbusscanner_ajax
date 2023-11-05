#include "Recipe.h"
#include "TouchPanel.h"
#include "SDCardHandler.h" // If needed for file operations
#include "FileManagement.h"


extern const int Y_AXIS_PADDING = 2;
extern const int X_LABEL_OFFSET = 35;
extern const int Y_LABEL_OFFSET = 130;
extern const int TEMP_LABEL_Y_OFFSET = 100;
extern std::vector<lv_obj_t*> x_axis_labels;
extern std::vector<Recipe> recipes;
extern std::vector<lv_obj_t*> temp_labels;
// Implementations of readRecipesFromFile, updateChartBasedOnRecipe, updateChartLabels, and clearLabels

lv_obj_t* createLabel(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

// Funktion zum Einlesen der CSV-Datei
void readRecipesFromFile() {
    File file = LittleFS.open("/kurven.csv", "r");
    if (!file) {
        Serial.println("Datei konnte nicht geöffnet werden");
        return;
    }
    file.readStringUntil('\n'); // Kopfzeile überspringen, falls vorhanden
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim(); // Entfernt Whitespace und Zeilenumbrüche
        if (line.length() == 0) {
            continue; // Leere Zeile überspringen
        }
        int delimiterPos = 0;
        Recipe recipe;
        // Rezeptnamen extrahieren
        delimiterPos = line.indexOf(';');
        recipe.name = line.substring(0, delimiterPos);
        line = line.substring(delimiterPos + 1); // Entferne den Rezeptnamen und das Semikolon
        // Temperaturen extrahieren
        while ((delimiterPos = line.indexOf(';')) != -1) {
            String tempStr = line.substring(0, delimiterPos);
            line = line.substring(delimiterPos + 1);
            recipe.temperatures.push_back(tempStr.toInt());
        }
        // Füge die letzte Temperatur hinzu (nach dem letzten Semikolon)
        if (line.length() > 0) {
            recipe.temperatures.push_back(line.toInt());
        }
        // Füge das Rezept zum Vektor hinzu
        recipes.push_back(recipe);
    }
    file.close();
}

// Funktion zum Aktualisieren des Charts basierend auf einem Rezept
void updateChartBasedOnRecipe(const Recipe& recipe) {
    if (!chart || !ser) {
        Serial.println("Chart oder Serie ist nullptr"); // Debugging-Ausgabe
        return;
    }

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


// Update the chart labels to be children of the chart container
// Update the chart labels to be children of the chart container
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart) {
    clearLabels(x_axis_labels);
    clearLabels(temp_labels);

    const lv_coord_t chart_width = lv_obj_get_width(chart);
    const lv_coord_t chart_height = lv_obj_get_height(chart);
    const lv_coord_t chart_x = lv_obj_get_x(chart);
    const lv_coord_t chart_y = lv_obj_get_y(chart);

    // Indices for significant points (start, middle, end)
    std::vector<size_t> significantPoints = {0, recipe.temperatures.size() / 2, recipe.temperatures.size() - 1};

    for (auto i : significantPoints) {
        char buf[10];
        snprintf(buf, sizeof(buf), "%zu", i);
        int x_offset = chart_x + (i * chart_width) / recipe.temperatures.size() + X_LABEL_OFFSET;
        int y_offset = chart_y + chart_height + Y_LABEL_OFFSET;
        x_axis_labels.push_back(createLabel(chart, buf, x_offset, y_offset));
    }

    // Create temperature labels
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    float y_scale = (float)chart_height / (max_temp - min_temp);
    for (auto i : significantPoints) {
        char buf[10];
        snprintf(buf, sizeof(buf), "%d", recipe.temperatures[i]);
        int x_pos = chart_x + (i * chart_width) / recipe.temperatures.size() + X_LABEL_OFFSET;
        int y_pos = chart_y + chart_height - ((recipe.temperatures[i] - min_temp) * y_scale) + TEMP_LABEL_Y_OFFSET;
        temp_labels.push_back(createLabel(chart, buf, x_pos, y_pos));
    }
}

void clearLabels(std::vector<lv_obj_t*>& labels) {
    for (auto& label : labels) {
        if (label) {
            lv_obj_del(label);
            label = nullptr; // Set the pointer to nullptr after deletion
        }
    }
    labels.clear();
}
