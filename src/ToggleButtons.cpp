#include "ToggleButtons.h"
#include "Process.h"
#include "SDCardHandler.h"
#include "Recipe.h"
#include <Preferences.h>
#include "DateTimeHandler.h"
#include "DatabaseHandler.h"
#include "ChartHandler.h"
#include "TouchPanel.h"
#include "WLANSettings.h"

extern WLANSettings wlanSettings;
extern SDCardHandler sdCard;
extern DateTime now;
extern std::vector<Recipe> recipes;
lv_obj_t* recipe_dropdown = nullptr;
lv_chart_series_t* progress_ser = nullptr; // Datenreihe für den Fortschritt
extern int selectedRecipeIndex;
extern bool coolingProcessRunning;
extern Preferences preferences;
extern int logintervall;
extern int stepsperday;
lv_obj_t* toggle_btn = nullptr;
extern  lv_style_t save_btn_style;
lv_obj_t* label = nullptr;
bool dropdown_exists = false;
lv_obj_t* btn = nullptr;

void updateToggleCoolingButtonText();

void createToggleCoolingButton(lv_obj_t * parent) {
    toggle_btn = lv_btn_create(parent);
    lv_obj_align(toggle_btn, LV_ALIGN_OUT_BOTTOM_MID, 175, 5); // Positionierung
    lv_obj_add_event_cb(toggle_btn, toggle_cooling_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_style(toggle_btn, &save_btn_style, 0); // Stil auf den Button anwenden
    lv_obj_set_size(toggle_btn, 100, 30); // Größe des Buttons anpassen
    label = lv_label_create(toggle_btn);
    lv_obj_center(label);
    lv_obj_set_user_data(toggle_btn, label); // Speichern Sie das Label im User-Daten des Buttons
}

void updateToggleCoolingButtonText() {
    if (!toggle_btn || !lv_obj_is_valid(toggle_btn)) {
        return;
    }
    label = reinterpret_cast<lv_obj_t*>(lv_obj_get_user_data(toggle_btn));
    if (!label || !lv_obj_is_valid(label)) {
        return;
    }
    lv_label_set_text(label, coolingProcessRunning ? "Stoppen" : "Starten");
}

void toggle_cooling_btn_event_cb(lv_event_t * e) {
    btn = lv_event_get_target(e);
    label = reinterpret_cast<lv_obj_t*>(lv_obj_get_user_data(btn));
    if (!coolingProcessRunning) {
        startCoolingProcess();
        updateToggleCoolingButtonText();
        coolingProcessRunning = true;
    } else {
        static const char* btns[] = {"OK", "Abbrechen", ""}; // Button-Beschriftungen
        lv_obj_t* mbox = lv_msgbox_create(lv_scr_act(), "Abbrechen bestaetigen", "AbkuehlProcess wirklich abbrechen?", btns, true);
        lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_add_event_cb(mbox, confirm_cooling_stop_cb, LV_EVENT_VALUE_CHANGED, mbox);
    }
}

void startCoolingProcess() {
    sdCard.deleteRowsWithCondition("Setpoints", 1700000000);

    if (selectedRecipeIndex < 0 || selectedRecipeIndex >= recipes.size()) {
        return;
    }

    coolingProcessRunning = true;
    updateToggleCoolingButtonText();
    const Recipe& selectedRecipe = recipes[selectedRecipeIndex];
    startTime = now.unixtime();
    unsigned long processDuration = (selectedRecipe.temperatures.size() - 1) * 24 * 60 * 60;
    unsigned long endTime = startTime + processDuration;
    displayEndTime(endTime);
    updateProgress();
    preferences.begin("process", false);
    preferences.putInt("coolingProcess", 1);
    preferences.putULong("endTime", endTime);
    preferences.putULong("starttime1", startTime);
    preferences.end();
}

void stopCoolingProcess() {
    coolingProcessRunning = false;
    preferences.begin("process", false);
    preferences.putInt("coolingProcess", 0);
    preferences.end();
    if (chart && progress_ser) {
        lv_chart_set_point_count(chart, lv_chart_get_point_count(chart));
        for (int i = 0; i < lv_chart_get_point_count(chart); i++) {
                lv_chart_set_next_value(chart, progress_ser, LV_CHART_POINT_NONE);
        }
    }
    const char* dbName = "/sd/setpoints.db";
    std::string tableName = "Setpoints";
    exportDataToXML(dbName, tableName, startTime);
    updateToggleCoolingButtonText();
}

void confirm_cooling_stop_cb(lv_event_t * e) {
    lv_obj_t* mbox = reinterpret_cast<lv_obj_t*>(lv_event_get_user_data(e));
    const char* btn_text = lv_msgbox_get_active_btn_text(mbox);
    if (strcmp(btn_text, "OK") == 0) {
        stopCoolingProcess();
    }
    lv_msgbox_close(mbox);
}

void createRecipeDropdown(lv_obj_t *parent)
{
    if (!parent || !lv_obj_is_valid(parent))
        return;
    if (!dropdown_exists)
    {
        recipe_dropdown = lv_dropdown_create(parent);
        lv_dropdown_clear_options(recipe_dropdown);
        for (size_t i = 0; i < recipes.size(); ++i)
        {
            lv_dropdown_add_option(recipe_dropdown, recipes[i].name.c_str(), i);
        }
        lv_dropdown_set_selected(recipe_dropdown, selectedRecipeIndex);
        lv_obj_set_size(recipe_dropdown, 150, 30); // Größe des Buttons anpassen
        lv_obj_align(recipe_dropdown, LV_ALIGN_TOP_MID, -45, 5);
        lv_obj_add_event_cb(recipe_dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_style(recipe_dropdown, &wlanSettings.style_no_border, 0);
        dropdown_exists = true;
    }
}

void updateRecipeDropdownState() {
    if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
        if (coolingProcessRunning == true) {
            lv_obj_add_state(recipe_dropdown, LV_STATE_DISABLED);
        } else {
            lv_obj_clear_state(recipe_dropdown, LV_STATE_DISABLED);
        }
    }
}

void recipe_dropdown_event_handler(lv_event_t * e) {
    recipe_dropdown = lv_event_get_target(e);
    if (!recipe_dropdown || !lv_obj_is_valid(recipe_dropdown)) return;

    int newSelectedIndex = lv_dropdown_get_selected(recipe_dropdown);
    if (newSelectedIndex != selectedRecipeIndex && !recipes.empty()) {
        selectedRecipeIndex = newSelectedIndex;
        sdCard.deleteRowsWithCondition("Setpoints", 0);
        sdCard.prepareInsertStatement("Setpoints");
        sdCard.beginTransaction();
        if (!sdCard.prepareInsertStatement("Setpoints")) {
        Serial.println("Failed to prepare insert statement");
        return;
        }
        const Recipe& selectedRecipe = recipes[selectedRecipeIndex];
        int stepDurationInSeconds = logintervall * 60 * 60; 
        for (size_t day = 0; day < selectedRecipe.temperatures.size(); day++) {
            float startTemp = selectedRecipe.temperatures[day];
            float endTemp = (day < selectedRecipe.temperatures.size() - 1) ? selectedRecipe.temperatures[day + 1] : startTemp;

            for (int step = 0; step < stepsperday; step++) {
                if (day == selectedRecipe.temperatures.size() - 1 && step > 0) break;

                float tempValue = (day < selectedRecipe.temperatures.size() - 1) ? 
                                  startTemp + ((endTemp - startTemp) * step / stepsperday) : 
                                  startTemp;

                unsigned long stepTime = 0 + day * 24 * 60 * 60 + step * stepDurationInSeconds;
                if (!sdCard.logSetpointData(stepTime, tempValue * 1000)) {
                    Serial.println("Error logging data to database");
                    break;
                }
            }
        }
        sdCard.endTransaction();

        if (chart && lv_obj_is_valid(chart)) {
            saveSelectedRecipe();
            updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
        }
    }
}