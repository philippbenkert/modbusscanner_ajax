#ifndef RECIPE_H
#define RECIPE_H

#include <vector>
#include <WString.h>
#include "lvgl.h"

struct Recipe {
    String name;
    std::vector<int> temperatures;
};

void readRecipesFromFile(std::vector<Recipe>& recipes);
void updateChartBasedOnRecipe(const Recipe& recipe, lv_obj_t* chart, lv_chart_series_t* ser);
void updateChartLabels(const Recipe& recipe, lv_obj_t* chart);
void clearLabels(std::vector<lv_obj_t*>& labels);

#endif // RECIPE_H
