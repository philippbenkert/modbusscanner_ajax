#include "Recipe.h"
#include "TouchPanel.h"
#include "SDCardHandler.h" // If needed for file operations
#include "FileManagement.h"

static lv_chart_cursor_t* cursor = nullptr; // Globale oder statische Variable für den Cursor
const int Y_AXIS_PADDING = 2;
const int Y_LABEL_OFFSET = 30;
extern std::vector<Recipe> recipes;
std::vector<lv_obj_t*> x_axis_labels;
std::vector<lv_obj_t*> temp_labels;
lv_obj_t* cursor_info_label;
extern lv_obj_t* recipe_dropdown;

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
    String line;
    line.reserve(256); // Reserviere genug Speicher für die Zeile, um häufige Allokationen zu vermeiden
    file.readStringUntil('\n'); // Überspringe die Kopfzeile
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
            continue; // Überspringe leere Zeilen
        }
        Recipe recipe;
        int start = 0;
        int end = line.indexOf(';');
        recipe.name = line.substring(start, end); // Extrahiere den Rezeptnamen
        start = end + 1;
        while ((end = line.indexOf(';', start)) != -1) {
            recipe.temperatures.push_back(line.substring(start, end).toInt()); // Extrahiere und konvertiere die Temperatur
            start = end + 1;
        }
        if (start < line.length()) {
            recipe.temperatures.push_back(line.substring(start).toInt()); // Verarbeite das letzte Stück der Zeile
        }
        recipes.push_back(std::move(recipe)); // Verwende std::move, um Kopien zu vermeiden
    }
    file.close();
}

const Recipe& getCurrentRecipe() {
    uint16_t selected_id = lv_dropdown_get_selected(recipe_dropdown);
    if (selected_id < recipes.size()) {
        return recipes[selected_id];
    } else {
        static Recipe default_recipe;
        return default_recipe;
    }
}

void updateCursorInfo(lv_obj_t* chart, const Recipe& recipe, uint16_t point_idx) {
    if (point_idx >= recipe.temperatures.size()) {
        return; // Sicherstellen, dass der Index im Bereich liegt
    }
    int temp = recipe.temperatures[point_idx];
    char buf[64];
    snprintf(buf, sizeof(buf), "Tag %d: %d°C", point_idx, temp);
    lv_label_set_text(cursor_info_label, buf);
    lv_obj_clear_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN); // Zeige das Label an
}

void chart_touch_event_cb(lv_event_t* e) {
    chart = lv_event_get_target(e);
    if (!chart || !lv_obj_is_valid(chart)) {
        Serial.print("Chart nicht vorhanden!");
        return; // Beenden, wenn das Chart-Objekt ungültig ist
    }
    if (!cursor) {
        return; // Beenden, wenn der Cursor ungültig ist
    }
    const Recipe& current_recipe = getCurrentRecipe(); // Verwenden Sie die neue Funktion
    uint32_t point_id = lv_chart_get_pressed_point(chart);
    if (point_id == LV_CHART_POINT_NONE) {
        return; // Beenden, wenn kein Punkt gedrückt wurde
    }
    lv_point_t p_out;
    lv_chart_get_point_pos_by_id(chart, ser, point_id, &p_out);
    
    if (point_id != LV_CHART_POINT_NONE) {
    lv_chart_set_cursor_point(chart, cursor, ser, point_id);
    }    
    updateCursorInfo(chart, current_recipe, point_id);
    if(chart && lv_obj_has_class(chart, &lv_chart_class)) {
    lv_chart_refresh(chart);
}
}

void clearCursor() {
    if (cursor) {
        //lv_obj_del((lv_obj_t*)cursor);
        cursor = nullptr;
        if(chart && lv_obj_has_class(chart, &lv_chart_class)) {
        lv_chart_refresh(chart);
        }
    }
}

void updateChartBasedOnRecipe(const Recipe& recipe) {
    clearContentArea();
    createRecipeDropdown(content_container);
    createSaveButton(content_container); // Button hinzufügen
    int X_MAX = recipe.temperatures.size(); // Maximale X-Position basierend auf der Anzahl der Temperaturen
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    if (!chart || !lv_obj_is_valid(chart)) {
        chart = lv_chart_create(content_container);
        lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
        lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -5);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        cursor_info_label = lv_label_create(chart);
        lv_label_set_text(cursor_info_label, ""); // Kein Text zu Beginn
        lv_obj_set_size(cursor_info_label, 100, 20); // Passen Sie die Größe nach Bedarf an
        lv_obj_align(cursor_info_label, LV_ALIGN_TOP_RIGHT, -10, 10); // Rechts oben im Chart positionieren
        lv_obj_add_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN); // Verstecken, bis benötigt
        if (chart && lv_obj_is_valid(chart)) {
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        }
        if (!ser) return;
    }   
     // Löscht den Cursor, falls vorhanden
    clearCursor();
    if (!cursor) {
        cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_VER);
        if (cursor) {
        }
    }
    if (!line_chart || !lv_obj_is_valid(line_chart)) {
        line_chart = lv_chart_create(content_container);
        lv_obj_set_size(line_chart, TFT_WIDTH - 60, 170);
        lv_obj_align(line_chart, LV_ALIGN_BOTTOM_MID, 10, -5);
        lv_chart_set_type(line_chart, LV_CHART_TYPE_SCATTER);
        lv_obj_set_style_border_opa(line_chart, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_opa(line_chart, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(line_chart, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(line_chart, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_shadow_opa(line_chart, LV_OPA_TRANSP, 0);
    }
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
    lv_chart_set_point_count(chart, recipe.temperatures.size());
    if (line_chart && lv_obj_is_valid(line_chart)) {
    zero_line_ser = lv_chart_add_series(line_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    }
    lv_chart_set_value_by_id2(line_chart, zero_line_ser, 0, 0, 0); // X_MIN: Minimale X-Position
    lv_chart_set_value_by_id2(line_chart, zero_line_ser, 1, X_MAX, 0); // X_MAX: Maximale X-Position
    lv_chart_set_range(line_chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
    lv_chart_set_range(line_chart, LV_CHART_AXIS_PRIMARY_X, 0, recipe.temperatures.size());
    static lv_style_t style;
    lv_style_init(&style);
    // Setzen Sie die Textfarbe auf Weiß
    lv_style_set_text_color(&style, lv_color_black());
    // Wenden Sie den Stil auf das Chart an
    lv_obj_add_style(chart, &style, LV_PART_TICKS);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 1, 1, recipe.temperatures.size(), 1, true, 20);
    int y_major_tick_count = 10; // Anzahl der Haupt-Ticks auf der Y-Achse
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 2, 2, y_major_tick_count, 1, true, 25);
    lv_obj_add_event_cb(chart, chart_touch_event_cb, LV_EVENT_PRESSED, nullptr);
    lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
    for (auto& temp : recipe.temperatures) {
        lv_chart_set_next_value(chart, ser, temp);
    }
    if(chart && lv_obj_has_class(chart, &lv_chart_class)) {
    lv_chart_refresh(chart);
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