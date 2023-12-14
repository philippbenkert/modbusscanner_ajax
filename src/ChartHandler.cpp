#include "ChartHandler.h"
#include "Process.h"
#include "SDCardHandler.h" // If needed for file operations
#include "TouchPanel.h"
#include "WLANSettings.h"
#include "DatabaseHandler.h"
#include "Recipe.h"

lv_chart_cursor_t* cursor = nullptr; // Globale oder statische Variable für den Cursor
extern lv_obj_t* end_time_label;
extern lv_obj_t* recipe_dropdown;
extern void initAxisStyle();
extern lv_style_t style;
extern std::vector<Recipe> recipes;
extern SDCardHandler sdCard;

// Benutzerdefinierte Zeichenfunktion
static void custom_draw_series(lv_event_t * e) {
    lv_obj_draw_part_dsc_t * dsc = (lv_obj_draw_part_dsc_t *)e->param;

    // Anpassen der Größe der Datenpunkte
    if(dsc->part == LV_PART_ITEMS) {
        dsc->draw_area->x1 += 2;
        dsc->draw_area->y1 += 2;
        dsc->draw_area->x2 -= 2;
        dsc->draw_area->y2 -= 2;
    }
}
void create_chart(){
    static bool is_chart_style_initialized = false;
    static lv_style_t chart_style;

    chart = lv_chart_create(content_container);
    lv_obj_add_style(chart, &style_no_border, 0);
    lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -10);
    cursor_info_label = lv_label_create(chart);
    lv_label_set_text(cursor_info_label, "");
    lv_obj_set_size(cursor_info_label, 95, 30);
    lv_obj_align(cursor_info_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN);

    if (!is_chart_style_initialized) {
        lv_style_init(&chart_style);
        lv_style_set_line_width(&chart_style, 2); // Setzt die Linienbreite
        lv_style_set_line_rounded(&chart_style, true); // Aktiviert die Glättung
        is_chart_style_initialized = true;
    }

    if (chart && lv_obj_is_valid(chart)) {
        lv_obj_add_style(chart, &chart_style, LV_PART_INDICATOR);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        progress_ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

        // Registrieren der benutzerdefinierten Zeichenfunktion für jede Serie
        lv_obj_add_event_cb(chart, custom_draw_series, LV_EVENT_DRAW_PART_BEGIN, ser);
        lv_obj_add_event_cb(chart, custom_draw_series, LV_EVENT_DRAW_PART_BEGIN, progress_ser);
    }
}

void updateChartBasedOnRecipe(const Recipe& recipe) {
    clearContentArea();
    SDCardHandler::setDbPath("/setpoint.db");
    Serial.println("Aktuelle Datenbank: /setpoint.db");

    lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
    createRecipeDropdown(content_container);
    createToggleCoolingButton(content_container);

    // Lese Daten aus der Datenbank
    dbData = readDatabaseData("/setpoint.db", "Setpoints");

    int totalPoints = dbData.size();
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());

    if (!chart || !lv_obj_is_valid(chart)) {
        create_chart();
    }

    if (chart && lv_obj_is_valid(chart) && lv_obj_has_class(chart, &lv_chart_class)) {
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
        lv_chart_set_point_count(chart, totalPoints);
        clearCursor();
        if (!cursor) {
            cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_BOTTOM);
        }
        initAxisStyle();
        // Anpassung der X-Achsenbeschriftung entsprechend der Anzahl der Datenpunkte
        int numLabels = dbData.size() /4; 
        if (dbData.size() % 4 != 0) {
        numLabels += 1;
        }
        lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, numLabels, 1, true, 20);
        int y_major_tick_count = 10; 
        lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, y_major_tick_count, 1, true, 25);
        lv_obj_add_event_cb(chart, chart_touch_event_cb, LV_EVENT_PRESSED, nullptr);
        lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
        // Setze die Temperaturen aus der Datenbank
        for (int i = 0; i < dbData.size(); ++i) {
            lv_chart_set_next_value(chart, ser, dbData[i].temperature);
        }
        lv_obj_add_style(chart, &style, LV_PART_TICKS);
        lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
        lv_chart_refresh(chart);
    }
    if (coolingProcessRunning) {
        displayEndTime(savedEndTime);
    } else {
        coolingProcessRunning = false;
        displayEndTime(savedEndTime);
    }
    updateToggleCoolingButtonText();
}

// Funktion, um den Fortschrittschart zu aktualisieren
void updateProgressChart(lv_obj_t* chart, const std::vector<TimeTempPair>& data, unsigned long currentTime) {
    if (!chart || !progress_ser) {
        Serial.println("Chart oder Series nicht initialisiert.");
        return;
    }
    // Setze die Punkteanzahl auf die Gesamtanzahl der Datenpunkte
    lv_chart_set_point_count(chart, data.size());
    // Durchlaufe alle Datenpunkte und füge sie dem Chart hinzu
    for (size_t i = 0; i < data.size(); ++i) {
        const auto& dataPoint = data[i];
        if (dataPoint.time <= currentTime) {
            // Gültiger Datenpunkt
            lv_chart_set_next_value(chart, progress_ser, dataPoint.temperature);
        } else {
            // Setze leeren Wert für Punkte nach der aktuellen Zeit
            lv_chart_set_next_value(chart, progress_ser, LV_CHART_POINT_NONE);
        }
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
    if (point_id != LV_CHART_POINT_NONE) {
        updateCursorInfo(chart, point_id);
    }
}

void updateCursorInfo(lv_obj_t* chart, uint16_t point_idx) {
    // Stellen Sie sicher, dass der Index innerhalb der Grenzen von dbData liegt
    if (point_idx < dbData.size()) {
        float temp = dbData[point_idx].temperature;
        char buf[64];
        snprintf(buf, sizeof(buf), "Temperatur: %.1f°C", temp);
        lv_label_set_text(cursor_info_label, buf);
        lv_obj_clear_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN); // Zeige das Label an
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

void updateCursorVisibility(lv_obj_t* chart, bool visible) {
    static bool isDropdownVisible = false; // Speichert den aktuellen Zustand des Dropdowns

    // Prüft, ob eine Änderung des Zustands erforderlich ist
    if (visible != isDropdownVisible) {
        if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
            if (visible) {
                lv_obj_clear_flag(recipe_dropdown, LV_OBJ_FLAG_HIDDEN); // Macht das Dropdown sichtbar

                isDropdownVisible = true;
                lv_color_t seriesColor = coolingProcessRunning ? lv_color_make(192, 192, 192) : lv_palette_main(LV_PALETTE_GREEN);
                updateSeriesColor(chart, seriesColor);
                lv_chart_refresh(chart); // Aktualisieren Sie das Chart, um die Änderungen anzuzeigen
            } else {
                lv_obj_add_flag(recipe_dropdown, LV_OBJ_FLAG_HIDDEN); // Versteckt das Dropdown

                isDropdownVisible = false;
                lv_color_t seriesColor = coolingProcessRunning ? lv_color_make(192, 192, 192) : lv_palette_main(LV_PALETTE_GREEN);
                updateSeriesColor(chart, seriesColor);
                lv_chart_refresh(chart); // Aktualisieren Sie das Chart, um die Änderungen anzuzeigen
            }
        }

        if (end_time_label && lv_obj_is_valid(end_time_label)) {
            if (visible) {
                lv_obj_add_flag(end_time_label, LV_OBJ_FLAG_HIDDEN); // Versteckt das Dropdown     
            } else {
                lv_obj_clear_flag(end_time_label, LV_OBJ_FLAG_HIDDEN); // Macht das Dropdown sichtbar
            }
        }
        if (cursor && lv_obj_is_valid(chart)) {
            if (visible) {
            } else {
            lv_chart_set_cursor_point(chart, cursor, ser, LV_CHART_POINT_NONE);
            }
        }
    }
}

float interpolate(int dayIndex, int subIndex, const Recipe& recipe) {
    if (dayIndex >= recipe.temperatures.size() - 1) {
        return recipe.temperatures.back();
    }
    float startTemp = recipe.temperatures[dayIndex];
    float endTemp = recipe.temperatures[dayIndex + 1];
    float step = (endTemp - startTemp) / 4.0; // 12 Schritte pro Tag
    return startTemp + step * subIndex;
}