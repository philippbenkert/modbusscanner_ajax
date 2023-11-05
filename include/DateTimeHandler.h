#ifndef DATETIMEHANDLER_H
#define DATETIMEHANDLER_H

#include <RTClib.h>
#include "lvgl.h" // Include LVGL header where lv_obj_t is defined

extern lv_obj_t* dateTimeLabel;

extern lv_obj_t* yearRoller;
extern lv_obj_t* hourRoller;
extern lv_obj_t* minuteRoller;
extern bool dstEnabled;
extern bool isCurrentlyDST;
extern lv_obj_t* dayRoller;
extern lv_obj_t* monthRoller;

void adjustForDST();
bool checkDST(DateTime now);
String getDateTimeStr();
void setDateTimeRollersToCurrent();
void setup_datetime_update(lv_obj_t * dateTimeLabel);
void update_datetime_label(lv_timer_t * timer);
void saveDSTEnabled();
void loadDSTEnabled();
void updateDSTStatus();


#endif // DATETIMEHANDLER_H
