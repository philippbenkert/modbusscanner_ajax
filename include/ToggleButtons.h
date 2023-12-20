#ifndef TOGGLE_BUTTONS_H
#define TOGGLE_BUTTONS_H

#include "lvgl.h"

void createToggleCoolingButton(lv_obj_t* parent);
void updateToggleCoolingButtonText();
void toggle_cooling_btn_event_cb(lv_event_t* e);
void startCoolingProcess();
void stopCoolingProcess();
void confirm_cooling_stop_cb(lv_event_t* e);
void createRecipeDropdown(lv_obj_t* parent);
void updateRecipeDropdownState();
void recipe_dropdown_event_handler(lv_event_t* e);

#endif // TOGGLE_BUTTONS_H
