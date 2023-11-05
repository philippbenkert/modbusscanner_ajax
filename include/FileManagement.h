#ifndef FILEMANAGEMENT_H
#define FILEMANAGEMENT_H

#include "ContentDrawer.h"
#include "lvgl.h"
#include "Recipe.h"
#include <vector>

// Deklarationen und Prototypen
extern lv_obj_t* chart;
extern lv_chart_series_t* ser;
extern std::vector<Recipe> recipes;
extern int selectedRecipeIndex;
extern bool is_update_timer_active;
extern lv_timer_t* update_timer;


void fileManagementFunction(lv_event_t *e);
void readRecipesFromFile();
void recipe_dropdown_event_handler(lv_event_t* e);
void chart_layout_updated_handler(lv_event_t* e);
void chart_ready_handler(lv_event_t* e);
void updateChartTimerCallback(lv_timer_t* timer);
void updateChartBasedOnRecipe(const Recipe& recipe);
void clearLabels(std::vector<lv_obj_t*>& labels);
void createRecipeDropdown(lv_obj_t* parent);

#endif
