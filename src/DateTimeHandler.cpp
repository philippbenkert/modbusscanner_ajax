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
  Serial.print("Loaded dstEnabled value from NVS: ");
  Serial.println(dstEnabled);
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
    if (!dstEnabled) {
        return; // Wenn DST deaktiviert ist, mache nichts
    }

    DateTime now = rtc.now();
    bool isDst = checkDST(now);

    // Prüfe, ob eine Anpassung notwendig ist
    if (isCurrentlyDST != isDst) {
        if (isDst) {
            // Beginn der Sommerzeit, füge eine Stunde hinzu
            now = now + TimeSpan(0, 1, 0, 0);
        } else {
            // Ende der Sommerzeit, subtrahiere eine Stunde
            now = now - TimeSpan(0, 1, 0, 0);
        }
        rtc.adjust(now);
        isCurrentlyDST = isDst; // Aktualisiere den gespeicherten DST-Status
    }
}

bool checkDST(DateTime now) {
    // Die EU wechselt zur Sommerzeit am letzten Sonntag im März
    // und zurück zur Winterzeit am letzten Sonntag im Oktober.
    // Hier ist eine vereinfachte Prüfung, die nur die Monate betrachtet.

    if (now.month() < 3 || now.month() > 10) return false; // Keine DST von November bis Februar
    if (now.month() > 3 && now.month() < 10) return true; // DST von April bis September

    // Für März und Oktober benötigen wir eine genauere Prüfung:
    // Finde den letzten Sonntag im Monat
    int day = (31 - (DateTime(now.year(), now.month(), 31).dayOfTheWeek() + 1)) % 7;
    DateTime lastSunday(now.year(), now.month(), 31 - day);

    if (now.month() == 3) {
        // Wenn das aktuelle Datum nach dem letzten Sonntag im März ist, ist DST aktiv
        return now >= lastSunday;
    } else if (now.month() == 10) {
        // Wenn das aktuelle Datum vor dem letzten Sonntag im Oktober ist, ist DST aktiv
        return now < lastSunday;
    }

    return false; // Standardmäßig keine DST
}

String getDateTimeStr() {
  DateTime now = getRTCDateTime();

  if (!now.isValid()) {
    Serial.println("RTC is not running!");
    return "RTC Error";
  }

  char dateTimeStr[20];
  snprintf(dateTimeStr, sizeof(dateTimeStr), "%02d.%02d.%04d %02d:%02d",
           now.day(), now.month(), now.year(), now.hour(), now.minute());

  // Debug-Ausgabe auf der Konsole
  Serial.print("RTC DateTime String: ");
  Serial.println(dateTimeStr);

  return String(dateTimeStr);
}

void setDateTimeRollersToCurrent() {
    DateTime now = rtc.now(); // Annahme, dass die Funktion `.now()` das aktuelle DateTime-Objekt vom RTC zurückgibt

    if (!now.isValid()) {
        Serial.println("RTC is not running!");
        return;
    }

    // Setzen der Roller auf den aktuellen Wert
    lv_roller_set_selected(dayRoller, now.day() - 1, LV_ANIM_OFF);
    lv_roller_set_selected(monthRoller, now.month() - 1, LV_ANIM_OFF);
    lv_roller_set_selected(yearRoller, now.year() - 2020, LV_ANIM_OFF); // Wenn die Roller-Jahre bei 2020 beginnen
    lv_roller_set_selected(hourRoller, now.hour(), LV_ANIM_OFF);
    lv_roller_set_selected(minuteRoller, now.minute(), LV_ANIM_OFF);
}

void setup_datetime_update(lv_obj_t * dateTimeLabel) {
    // 'dateTimeLabel' ist das lv_obj_t* für das Datum- und Zeit-Label
    if (dateTimeLabel != nullptr) {
        lv_timer_t * timer = lv_timer_create(update_datetime_label, 1000, dateTimeLabel); // 1000 ms (1 Sekunde) Intervall
        lv_timer_ready(timer); // Startet den Timer sofort
    }
}
