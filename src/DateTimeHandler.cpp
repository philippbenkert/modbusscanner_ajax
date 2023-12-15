#include "DateTimeHandler.h"
#include <TimeLib.h>
#include "RTCControl.h"
#include <Preferences.h>
#include "StatusDisplay.h"


extern RTC_DS3231 rtc;

Preferences preferences;

void saveDSTEnabled() {
  Serial.println("Saving dstEnabled value to NVS");
  preferences.begin("settings", false);
  preferences.putBool("dstEnabled", dstEnabled);
  preferences.end();
}

void loadDSTEnabled() {
    preferences.begin("settings", true);
    dstEnabled = preferences.getBool("dstEnabled", false);
    Serial.printf("Loaded dstEnabled value from NVS: %s\n", dstEnabled ? "true" : "false");
    preferences.end();
}

void updateDSTStatus() {
  if (dstSwitch != nullptr) {
    if (dstEnabled) {
      lv_obj_add_state(dstSwitch, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(dstSwitch, LV_STATE_CHECKED);
    }
  }
}

void adjustForDST() {
    if (!dstEnabled) return;

    DateTime now = rtc.now();
    bool isDst = checkDST(now);

    if (isCurrentlyDST != isDst) {
        TimeSpan adjustmentTime(0, isDst ? 1 : -1, 0, 0);
        rtc.adjust(now + adjustmentTime);
        isCurrentlyDST = isDst;
    }
}

bool checkDST(DateTime now) {
    if (now.month() < 3 || now.month() > 10) return false; // Keine DST von November bis Februar
    if (now.month() > 3 && now.month() < 10) return true; // DST von April bis September

    // Berechnung des letzten Sonntags im Monat
    DateTime lastSunday(now.year(), now.month(), 1);
    lastSunday = lastSunday + TimeSpan(31 - lastSunday.dayOfTheWeek(), 0, 0, 0);
    if (lastSunday.month() != now.month()) {
        lastSunday = lastSunday - TimeSpan(7, 0, 0, 0);
    }

    return (now.month() == 3) ? (now >= lastSunday) : (now < lastSunday);
}

String getDateTimeStr() {
    DateTime now = rtc.now();
    if (!now.isValid()) {
        Serial.println("RTC is not running!");
        return "RTC Error";
    }

    char dateTimeStr[20];
    snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d.%02d.%04d %02d:%02d",
             now.day(), now.month(), now.year(), now.hour(), now.minute());
    return String(dateTimeStr);
}

void setDateTimeRollersToCurrent() {
    DateTime now = rtc.now();
    if (!now.isValid()) {
        Serial.println("RTC is not running!");
        return;
    }

    lv_roller_set_selected(dayRoller, now.day() - 1, LV_ANIM_OFF);
    lv_roller_set_selected(monthRoller, now.month() - 1, LV_ANIM_OFF);
    lv_roller_set_selected(yearRoller, now.year() - 2020, LV_ANIM_OFF);
    lv_roller_set_selected(hourRoller, now.hour(), LV_ANIM_OFF);
    lv_roller_set_selected(minuteRoller, now.minute(), LV_ANIM_OFF);
}

void setup_datetime_update(lv_obj_t* dateTimeLabel) {
    if (dateTimeLabel) {
        lv_timer_t* timer = lv_timer_create(update_datetime_label, 1000, dateTimeLabel);
        lv_timer_ready(timer);
    }
}