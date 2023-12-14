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
extern unsigned long startCoolingTime;

void ProcessFunction(lv_event_t *e);
void readRecipesFromFile();
void recipe_dropdown_event_handler(lv_event_t* e);
void updateChartBasedOnRecipe(const Recipe& recipe);
void clearLabels(std::vector<lv_obj_t*>& labels);
void createRecipeDropdown(lv_obj_t* parent);
void createToggleCoolingButton(lv_obj_t * parent);
void loadCoolingProcessStatus();
void updateToggleCoolingButtonText();
void displayEndTime(unsigned long endTime);
void updateRecipeDropdownState();

#endif
