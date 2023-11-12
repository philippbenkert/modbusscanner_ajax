#include "Recipe.h"
#include "TouchPanel.h"
#include "SDCardHandler.h" // If needed for file operations
#include "FileManagement.h"

const int Y_AXIS_PADDING = 2;
const int Y_LABEL_OFFSET = 30;
extern std::vector<Recipe> recipes;
std::vector<lv_obj_t*> x_axis_labels;
std::vector<lv_obj_t*> temp_labels;

lv_obj_t* createLabel(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* label = lv_label_create(parent);
    if (!label) return nullptr;
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

void readRecipesFromFile() {
    File file = LittleFS.open("/kurven.csv", "r");
    if (!file) {
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
        delimiterPos = line.indexOf(';');
        recipe.name = line.substring(0, delimiterPos);
        line = line.substring(delimiterPos + 1); // Entferne den Rezeptnamen und das Semikolon
        while ((delimiterPos = line.indexOf(';')) != -1) {
            String tempStr = line.substring(0, delimiterPos);
            line = line.substring(delimiterPos + 1);
            recipe.temperatures.push_back(tempStr.toInt());
        }
        if (line.length() > 0) {
            recipe.temperatures.push_back(line.toInt());
        }
        recipes.push_back(recipe);
    }
    file.close();
}

void updateChartBasedOnRecipe(const Recipe& recipe) {
    if (!chart || !lv_obj_is_valid(chart)) {
        chart = lv_chart_create(content_container);
        lv_obj_set_size(chart, TFT_WIDTH - 20, 150);
        lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, recipe.temperatures.size());
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -24, 20);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);

        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
        if (!ser) return; // Verlasse die Funktion, falls die Serie nicht hinzugefügt werden kann

        }
    // Setze den Bereich und die Daten für das Diagramm
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
    lv_chart_set_point_count(chart, recipe.temperatures.size());
    lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
    // Rasterlinien für bestimmte Temperaturen
    lv_chart_set_div_line_count(chart, 3, 0); // 3 Y-Unterteilungen für die Temperaturen +20, 0, -20
    lv_obj_set_style_line_color(chart, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_ITEMS); // Linienfarbe auf Gelb setzen
    
    for (size_t i = 0; i < recipe.temperatures.size(); i++) {
        lv_chart_set_next_value(chart, ser, recipe.temperatures[i]);
    }

    lv_chart_refresh(chart);
    updateChartLabels(recipe, chart);
}

void updateChartLabels(const Recipe& recipe, lv_obj_t* chart) {
    if (!chart || !lv_obj_is_valid(chart)) return;
    clearLabels(x_axis_labels);
    clearLabels(temp_labels);
    const lv_coord_t chart_width = lv_obj_get_width(chart);
    const lv_coord_t chart_height = lv_obj_get_height(chart);
    const lv_coord_t chart_x = lv_obj_get_x(chart);
    const lv_coord_t chart_y = lv_obj_get_y(chart);
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    float y_scale = (float)(chart_height -20) / (max_temp - min_temp);

        for (size_t i = 0; i < recipe.temperatures.size(); i++) {
        // Berechnung der X- und Y-Positionen für Labels
        int x_pos = chart_x + (i * chart_width) / recipe.temperatures.size() + 5;
        int y_pos_for_day = chart_y + chart_height + Y_LABEL_OFFSET;
        int y_pos_for_temp = chart_y + chart_height - 20 - ((recipe.temperatures[i] - min_temp) * y_scale) - 100;
        // Erstelle und positioniere Labels
        char day_buf[10];
        snprintf(day_buf, sizeof(day_buf), "%u", (unsigned int)i);
        x_axis_labels.push_back(createLabel(content_container, day_buf, x_pos, y_pos_for_day));
        // Erstelle und positioniere Y-Achsen-Label (Temperaturen)
        char temp_buf[10];
        snprintf(temp_buf, sizeof(temp_buf), "%d", recipe.temperatures[i]);
        temp_labels.push_back(createLabel(chart, temp_buf, x_pos + 5, y_pos_for_temp));
    }
}

void clearLabels(std::vector<lv_obj_t*>& labels) {
    for (auto& label : labels) {
        if (label && lv_obj_is_valid(label)) {
            lv_obj_del(label);
            label = nullptr;
        }
    }
    labels.clear();
}