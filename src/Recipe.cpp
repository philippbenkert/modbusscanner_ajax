#include "Recipe.h"
#include "TouchPanel.h"
#include "SDCardHandler.h" // If needed for file operations
#include "FileManagement.h"

extern const int Y_AXIS_PADDING = 2;
extern const int X_LABEL_OFFSET = 35;
extern const int Y_LABEL_OFFSET = 0;
extern const int TEMP_LABEL_Y_OFFSET = 0;
extern std::vector<lv_obj_t*> x_axis_labels;
extern std::vector<Recipe> recipes;
extern std::vector<lv_obj_t*> temp_labels;
extern bool pendingUpdate;

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
    // Überprüfe, ob chart und ser gültig sind, bevor du fortfährst.
    if (!chart || !lv_obj_is_valid(chart)) {
        Serial.println("Chart ist ungültig");
        return;
    }

    // Ensure the series is valid
    if (!ser) {
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
        if (!ser) {
            Serial.println("Serie konnte nicht hinzugefügt werden");
            return;
        }
    }

    // Set the data range for the x-axis and y-axis
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, recipe.temperatures.size());
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
    
    // Set the number of points that the chart will hold
    lv_chart_set_point_count(chart, recipe.temperatures.size());

    // Clear existing data points in the series
    lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);

    // Add new data points
    for (size_t i = 0; i < recipe.temperatures.size(); i++) {
        lv_chart_set_next_value(chart, ser, recipe.temperatures[i]);
    }

    // Refresh the chart
    lv_chart_refresh(chart);

    // Update chart labels
    updateChartLabels(recipe, chart);
    pendingUpdate = false;

}



// Update the chart labels to be children of the chart container
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart) {
    if (!lv_obj_is_valid(chart)) {
        Serial.println("Chart object is invalid");
        return; // Exit the function if the chart is not valid
    }

    clearLabels(x_axis_labels);
    clearLabels(temp_labels);

    const lv_coord_t chart_width = lv_obj_get_width(chart);
    const lv_coord_t chart_height = lv_obj_get_height(chart);
    const lv_coord_t chart_x = lv_obj_get_x(chart);
    const lv_coord_t chart_y = lv_obj_get_y(chart);

    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    float y_scale = (float)chart_height / (max_temp - min_temp);
    // Helper function to create and position labels
    auto createAndPositionLabel = [&](size_t index, bool isTempLabel) {
        char buf[10];
        int value = isTempLabel ? recipe.temperatures[index] : static_cast<int>(index);
        snprintf(buf, sizeof(buf), "%d", value);
        int x_offset = chart_x + (index * chart_width) / recipe.temperatures.size() + X_LABEL_OFFSET;
        int y_offset = isTempLabel ? (chart_y + chart_height - ((recipe.temperatures[index] - min_temp) * y_scale) + TEMP_LABEL_Y_OFFSET)
                                   : (chart_y + chart_height + Y_LABEL_OFFSET);
        auto& labelVector = isTempLabel ? temp_labels : x_axis_labels;
        labelVector.push_back(createLabel(chart, buf, x_offset, y_offset));
        
    };

    // Indices for significant points (start, middle, end)
    std::vector<size_t> significantPoints = {0, recipe.temperatures.size() / 2, recipe.temperatures.size() - 1};

    for (auto i : significantPoints) {
        createAndPositionLabel(i, false); // X-axis labels
        createAndPositionLabel(i, true);  // Temperature labels
    }
}

void clearLabels(std::vector<lv_obj_t*>& labels) {
    for (auto& label : labels) {
        if (label && lv_obj_is_valid(label)) {
            lv_obj_del(label);
            label = nullptr; // Set the pointer to nullptr after deletion
        }
    }
    labels.clear();
}
