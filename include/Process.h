#ifndef Process_H
#define Process_H

#include "ContentDrawer.h"
#include "lvgl.h"
#include "Recipe.h"
#include <vector>

// Deklarationen und Prototypen
extern std::vector<Recipe> recipes;
extern int selectedRecipeIndex;
extern lv_obj_t* chart;
extern lv_chart_series_t* ser;
extern lv_chart_series_t* progress_ser;
extern unsigned long savedEndTime;
extern bool coolingProcessRunning;
extern unsigned long startTime;

extern void saveSelectedRecipe();
void ProcessFunction(lv_event_t *e);
void readRecipesFromFile();
void updateChartBasedOnRecipe(const Recipe& recipe);
void clearLabels(std::vector<lv_obj_t*>& labels);
void loadCoolingProcessStatus();
void displayEndTime(unsigned long endTime);

#endif
