#ifndef CHARThANDLER_H
#define CHARThANDLER_H

#include "lvgl.h"
#include "Recipe.h"
#include <vector>
#include "CommonDefinitions.h"

extern lv_obj_t* chart;
extern lv_chart_series_t* ser;
extern lv_chart_series_t* progress_ser;
extern lv_chart_cursor_t* cursor;
extern lv_obj_t* cursor_info_label;
extern bool coolingProcessRunning;

void createChart();
void updateChartBasedOnRecipe(const Recipe& recipe);
void updateProgressChart(lv_obj_t* chart, lv_chart_series_t* progress_ser, const std::vector<TimeTempPair>& data, unsigned long startCoolingTime);
void chart_touch_event_cb(lv_event_t* e);
void updateCursorInfo(lv_obj_t* chart, uint16_t point_idx);
void updateCursorVisibility(lv_obj_t* chart, bool visible);
float interpolate(int dayIndex, int subIndex, const Recipe& recipe);

#endif // CHARThANDLER_H
