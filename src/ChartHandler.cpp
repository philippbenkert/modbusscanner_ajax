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

static void customDrawSeries(lv_event_t* e) {
    auto* dsc = static_cast<lv_obj_draw_part_dsc_t*>(e->param);
    if (dsc->part == LV_PART_ITEMS) {
        dsc->draw_area->x1 += 2;
        dsc->draw_area->y1 += 2;
        dsc->draw_area->x2 -= 2;
        dsc->draw_area->y2 -= 2;
    }
}
void createChart() {
    static bool isChartStyleInitialized = false;
    static lv_style_t chartStyle;

    chart = lv_chart_create(content_container);
    lv_obj_add_style(chart, &style_no_border, 0);
    lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -10);
    cursor_info_label = lv_label_create(chart);
    lv_label_set_text(cursor_info_label, "");
    lv_obj_set_size(cursor_info_label, 95, 30);
    lv_obj_align(cursor_info_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN);

    if (!isChartStyleInitialized) {
        lv_style_init(&chartStyle);
        lv_style_set_line_width(&chartStyle, 2);
        lv_style_set_line_rounded(&chartStyle, true);
        isChartStyleInitialized = true;
    }

    if (chart && lv_obj_is_valid(chart)) {
        lv_obj_add_style(chart, &chartStyle, LV_PART_INDICATOR);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        progress_ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

        lv_obj_add_event_cb(chart, customDrawSeries, LV_EVENT_DRAW_PART_BEGIN, ser);
        lv_obj_add_event_cb(chart, customDrawSeries, LV_EVENT_DRAW_PART_BEGIN, progress_ser);
    }
}

void updateChartBasedOnRecipe(const Recipe& recipe) {
    clearContentArea();
    lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
    createRecipeDropdown(content_container);
    createToggleCoolingButton(content_container);
    dbData = readDatabaseData("/sd/setpoint.db", "Setpoints");
    int totalPoints = dbData.size();
    int minTemp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int maxTemp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());
    if (!chart || !lv_obj_is_valid(chart)) {
        createChart();
    }
    if (chart && lv_obj_is_valid(chart)) {
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, minTemp - Y_AXIS_PADDING, maxTemp + Y_AXIS_PADDING);
        lv_chart_set_point_count(chart, totalPoints);
        clearCursor();
        cursor = cursor ? cursor : lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_BOTTOM);
        initAxisStyle();
        int numLabels = (dbData.size() + 3) / 4; // Ceiling division
        lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, numLabels, 1, true, 20);
        int yMajorTickCount = 10;
        lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, yMajorTickCount, 1, true, 25);
        lv_obj_add_event_cb(chart, chart_touch_event_cb, LV_EVENT_PRESSED, nullptr);
        lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
        for (const auto& dataPoint : dbData) {
            lv_chart_set_next_value(chart, ser, dataPoint.temperature); // Zugriff auf das 'temperature'-Feld von TimeTempPair
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

void updateProgressChart(lv_obj_t* chart, lv_chart_series_t* progress_ser, const std::vector<TimeTempPair>& data, unsigned long startCoolingTime) {
    if (!chart || !progress_ser) return;
    unsigned long currentTime = millis(); // Aktuelle Zeit in Millisekunden
    unsigned long elapsedTime = currentTime - startCoolingTime;
    lv_chart_set_point_count(chart, data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        const auto& dataPoint = data[i];
        unsigned long dataPointTime = startCoolingTime + i * 4 * 60 * 60 * 1000;
        if (dataPointTime <= currentTime) {
            lv_chart_set_next_value(chart, progress_ser, dataPoint.temperature);
        } else {
            lv_chart_set_next_value(chart, progress_ser, LV_CHART_POINT_NONE);
        }
    }
}

void chart_touch_event_cb(lv_event_t* e) {
    if (coolingProcessRunning || !cursor) return;
    chart = lv_event_get_target(e);
    if (!chart || !lv_obj_is_valid(chart)) return;
    
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

void updateCursorInfo(lv_obj_t* chart, uint16_t pointIdx) {
    if (pointIdx >= dbData.size()) return;
    float temp = dbData[pointIdx].temperature;
    char buf[64];
    snprintf(buf, sizeof(buf), "Temperatur: %.1f°C", temp);
    lv_label_set_text(cursor_info_label, buf);
    lv_obj_clear_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN);
}

void clearCursor() {
    if (!cursor) return;
    cursor = nullptr;
    if (chart && lv_obj_has_class(chart, &lv_chart_class)) {
        lv_chart_refresh(chart);
    }
}

void updateCursorVisibility(lv_obj_t* chart, bool visible) {
    static bool isDropdownVisible = false; // Speichert den aktuellen Zustand des Dropdowns
    if (visible == isDropdownVisible) return;
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

float interpolate(int dayIndex, int subIndex, const Recipe& recipe) {
    if (dayIndex >= recipe.temperatures.size() - 1) {
        return recipe.temperatures.back();
    }
    float startTemp = recipe.temperatures[dayIndex];
    float endTemp = recipe.temperatures[dayIndex + 1];
    float step = (endTemp - startTemp) / 4.00; // 12 Schritte pro Tag
    return startTemp + step * subIndex;
}