#include "Recipe.h"
#include "TouchPanel.h"
#include "SDCardHandler.h" // If needed for file operations
#include "FileManagement.h"
#include "WLANSettings.h"
#include "DateTimeHandler.h"

static lv_chart_cursor_t* cursor = nullptr; // Globale oder statische Variable für den Cursor
const int Y_AXIS_PADDING = 2;
const int Y_LABEL_OFFSET = 30;
extern std::vector<Recipe> recipes;
std::vector<lv_obj_t*> x_axis_labels;
std::vector<lv_obj_t*> temp_labels;
lv_obj_t* cursor_info_label;
extern lv_obj_t* recipe_dropdown;
static lv_style_t style;
static lv_style_t axis_style; // Stil für Achsenbeschriftungen
extern bool coolingProcessRunning;

extern std::vector<Recipe> recipes;

// Funktion, um den Fortschritt zu berechnen
int calculateCoolingProgress(unsigned long currentTime, unsigned long startCoolingTime, const Recipe& recipe) {
    unsigned long elapsedSeconds = currentTime - startCoolingTime;
    int daysProgressed = elapsedSeconds / 86400; // 86400 Sekunden pro Tag

    // Begrenze die Anzahl der Tage auf die Länge des Rezepts
    if (daysProgressed >= recipe.temperatures.size()) {
        daysProgressed = recipe.temperatures.size() - 1;
    }

    return daysProgressed;
}

// Funktion, um den Fortschrittschart zu aktualisieren
void updateProgressChart(lv_obj_t* chart, const Recipe& recipe, unsigned long currentTime) {
    if (!chart || !lv_obj_is_valid(chart)) return;
    if (!progress_ser) {
        progress_ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    }

    int daysProgressed = calculateCoolingProgress(currentTime, startCoolingTime, recipe);

    lv_chart_set_point_count(chart, recipe.temperatures.size());

    // Zeichne den blauen Graphen entlang der Sollwertkurve bis zum aktuellen Fortschritt
    for (size_t i = 0; i < recipe.temperatures.size(); i++) {
        int value = i <= daysProgressed ? recipe.temperatures[i] : LV_CHART_POINT_NONE;
        lv_chart_set_next_value(chart, progress_ser, value);
    }
}


lv_obj_t* createLabel(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* label = lv_label_create(parent);
    if (!label) return nullptr;
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

void initAxisStyle() {
    if (!initAxisStyle) {
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_make(255, 0, 0)); // Setzt die Textfarbe auf Schwarz
    }
}

void readRecipesFromFile() {
    File file = LittleFS.open("/kurven.csv", "r");
    if (!file) {
        return;
    }
    String line;
    line.reserve(128); // Reserviere genug Speicher für die Zeile, um häufige Allokationen zu vermeiden
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

void updateSeriesColor(lv_obj_t* chart, lv_color_t color) {
    if (ser && chart && lv_obj_is_valid(chart)) {
        lv_chart_set_series_color(chart, ser, color);
    }
}

void chart_touch_event_cb(lv_event_t* e) {
    if (coolingProcessRunning) {
        return;
    }
    chart = lv_event_get_target(e);
    if (!chart || !lv_obj_is_valid(chart)) {
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

void updateCursorVisibility(lv_obj_t* chart, bool visible) {
    if (cursor && lv_obj_is_valid((lv_obj_t*)cursor)) {
        if (visible) {
            // Wenn der Cursor sichtbar sein soll, fügen Sie ihn hinzu und aktivieren Sie den Event-Callback
            lv_obj_clear_flag((lv_obj_t*)cursor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(chart, chart_touch_event_cb, LV_EVENT_PRESSED, nullptr); // Reaktiviert den Callback
        } else {
            // Wenn der Cursor nicht sichtbar sein soll, verstecken Sie ihn und deaktivieren Sie den Event-Callback
            lv_obj_add_flag((lv_obj_t*)cursor, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_event_cb(chart, chart_touch_event_cb); // Entfernt den Callback
        }
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

lv_obj_t* createChart(lv_obj_t* parent, lv_chart_type_t chart_type, const Recipe& recipe, lv_style_t* style) {
    lv_obj_t* chart = lv_chart_create(parent);
    lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -5);
    lv_chart_set_type(chart, chart_type);
    lv_obj_add_style(chart, &style_no_border, 0);

    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end()) - Y_AXIS_PADDING;
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end()) + Y_AXIS_PADDING;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp, max_temp);

    if (chart_type == LV_CHART_TYPE_LINE) {
        lv_chart_set_point_count(chart, recipe.temperatures.size());
    }

    lv_obj_add_style(chart, style, 0);
    return chart;
}

void create_chart(){
    chart = lv_chart_create(content_container);
        lv_obj_add_style(chart, &style_no_border, 0);
        lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
        lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -10);
        cursor_info_label = lv_label_create(chart);
        lv_label_set_text(cursor_info_label, ""); // Kein Text zu Beginn
        lv_obj_set_size(cursor_info_label, 100, 20); // Passen Sie die Größe nach Bedarf an
        lv_obj_align(cursor_info_label, LV_ALIGN_TOP_RIGHT, -10, 10); // Rechts oben im Chart positionieren
        lv_obj_add_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN); // Verstecken, bis benötigt
        if (chart && lv_obj_is_valid(chart)) {
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        }
        if (!ser) return;
}
void updateChartBasedOnRecipe(const Recipe& recipe) {
    clearContentArea();
    lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
    createRecipeDropdown(content_container);
    createSaveButton(content_container); // Button hinzufügen
    createToggleCoolingButton(content_container);
    int X_MAX = recipe.temperatures.size(); // Maximale X-Position basierend auf der Anzahl der Temperaturen
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    if (!chart || !lv_obj_is_valid(chart)) {
        create_chart();
    }   
    if (chart && lv_obj_is_valid(chart) && lv_obj_has_class(chart, &lv_chart_class)) {
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
    lv_chart_set_point_count(chart, recipe.temperatures.size());
    }
     // Löscht den Cursor, falls vorhanden
    clearCursor();
    if (!cursor) {
        if (chart && lv_obj_is_valid(chart) && lv_obj_has_class(chart, &lv_chart_class)) {
        cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_VER);
        }
    }
    if(chart && lv_obj_has_class(chart, &lv_chart_class)) {
    initAxisStyle();
    // Wenden Sie den Stil auf das Chart an
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 1, 1, recipe.temperatures.size(), 1, true, 20);
    int y_major_tick_count = 10; // Anzahl der Haupt-Ticks auf der Y-Achse
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, y_major_tick_count, 1, true, 25);
    lv_obj_add_event_cb(chart, chart_touch_event_cb, LV_EVENT_PRESSED, nullptr);
    lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
    for (auto& temp : recipe.temperatures) {
        lv_chart_set_next_value(chart, ser, temp);
    }
    lv_obj_add_style(chart, &style, LV_PART_TICKS); // Wenden Sie den Stil auf die Achsenbeschriftungen an
    lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_chart_refresh(chart);
    }

    //DateTime now = rtc.now();
    if (coolingProcessRunning) {
        displayEndTime(savedEndTime);
    } else {
        coolingProcessRunning = false;
        displayEndTime(savedEndTime);
    }
    updateToggleCoolingButtonText();
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