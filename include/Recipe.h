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
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart);
void clearLabels(std::vector<lv_obj_t*>& labels);
void init_temp_line_style();

#endif // RECIPE_H
