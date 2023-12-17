#ifndef RECIPE_H
#define RECIPE_H

#include <vector>
#include <WString.h>
#include "lvgl.h"

struct Recipe {
    String name;
    std::vector<int> temperatures;
};

void readRecipesFromFile();
void updateChartBasedOnRecipe(const Recipe& recipe, lv_obj_t* chart, lv_chart_series_t* ser);
void init_temp_line_style();
void updateCursorVisibility(lv_obj_t* chart, bool visible);
void updateSeriesColor(lv_obj_t* chart, lv_color_t color);
const Recipe& getCurrentRecipe();

#endif // RECIPE_H
