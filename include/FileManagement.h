#ifndef FILEMANAGEMENT_H
#define FILEMANAGEMENT_H

#include "ContentDrawer.h"

// Deklarationen und Prototypen
extern lv_obj_t * slider1;
extern lv_obj_t * slider2;
extern lv_obj_t * chart;
extern lv_chart_series_t * ser;
extern lv_obj_t * value_label1;
extern lv_obj_t * value_label2;

void fileManagementFunction(lv_event_t *e);
void slider_event_cb(lv_event_t *e);
void readRecipesFromFile();


#endif
