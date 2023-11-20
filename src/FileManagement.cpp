#include "FileManagement.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"
#include "Recipe.h"
#include <Preferences.h>
#include "DateTimeHandler.h"

extern Preferences preferences;
std::vector<Recipe> recipes;
lv_obj_t* chart = nullptr;
lv_chart_series_t* ser = nullptr;
lv_obj_t* recipe_dropdown = nullptr;
lv_obj_t* line_chart = nullptr;
lv_chart_series_t* zero_line_ser = nullptr;


bool dropdown_exists = false;
int selectedRecipeIndex = 0;

void showSaveConfirmationPopup() {
    lv_obj_t* mbox = lv_msgbox_create(lv_scr_act(), "Gespeichert", "Das Rezept wurde erfolgreich gespeichert!", nullptr, true);
    lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mbox, LV_OPA_COVER, LV_PART_MAIN);
    // Setze einen Timer, um das Popup automatisch zu schließen
    lv_timer_t* timer = lv_timer_create([](lv_timer_t * timer) {
        lv_obj_t* mbox = reinterpret_cast<lv_obj_t*>(timer->user_data);
            if(lv_obj_is_valid(mbox)) { // Überprüfen Sie die Gültigkeit des Objekts
            lv_msgbox_close(mbox);
            }
            lv_timer_del(timer);
    }, 2000, mbox); // Schließt das Popup nach 2 Sekunden
}

void saveSelectedRecipe() {
    if (recipes.empty() || selectedRecipeIndex < 0 || selectedRecipeIndex >= recipes.size()) {
        return;
    }
    preferences.begin("recipe_app", false); // "false" für schreibzugriff
    preferences.putInt("selectedRecipe", selectedRecipeIndex);
    preferences.end();
}

void createSaveButton(lv_obj_t * parent) {
    if (!parent || !lv_obj_is_valid(parent)) return;

    // Erstellen Sie einen neuen Stil für den Button
    static lv_style_t save_btn_style;
    lv_style_init(&save_btn_style);
    lv_style_set_bg_color(&save_btn_style, lv_color_hex(0x00AEEF)); // Setzen Sie die gewünschte Hintergrundfarbe
    lv_style_set_bg_opa(&save_btn_style, LV_OPA_COVER); // Deckkraft auf Maximum setzen
    // Erstellen Sie den Button und wenden Sie den Stil an
    lv_obj_t* save_btn = lv_btn_create(parent);
    lv_obj_add_style(save_btn, &save_btn_style, 0); // Stil auf den Button anwenden
    lv_obj_set_size(save_btn, 100, 30); // Größe des Buttons anpassen
    lv_obj_align(save_btn, LV_ALIGN_OUT_BOTTOM_MID, 150, 12); // Position unterhalb des Dropdown-Menüs
    // Erstellen Sie das Label für den Button
    lv_obj_t* label = lv_label_create(save_btn);
    lv_label_set_text(label, "Speichern");
    // Event-Callback für den Button hinzufügen
    lv_obj_add_event_cb(save_btn, [](lv_event_t * e) {
        saveSelectedRecipe();
        showSaveConfirmationPopup();
    }, LV_EVENT_CLICKED, NULL);
}


void recipe_dropdown_event_handler(lv_event_t * e) {
    recipe_dropdown = lv_event_get_target(e);
    if (!recipe_dropdown || !lv_obj_is_valid(recipe_dropdown)) return;
    int newSelectedIndex = lv_dropdown_get_selected(recipe_dropdown);
    if (newSelectedIndex != selectedRecipeIndex && !recipes.empty()) {
        selectedRecipeIndex = newSelectedIndex;
        if (chart && lv_obj_is_valid(chart)) {
            updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
        }
    }
}

void createRecipeDropdown(lv_obj_t * parent) {
    if (!parent || !lv_obj_is_valid(parent)) return;
    if (!dropdown_exists) {
        recipe_dropdown = lv_dropdown_create(parent);
        lv_dropdown_clear_options(recipe_dropdown);
        for (size_t i = 0; i < recipes.size(); ++i) {
            lv_dropdown_add_option(recipe_dropdown, recipes[i].name.c_str(), i);
        }
        lv_dropdown_set_selected(recipe_dropdown, selectedRecipeIndex);
        lv_obj_set_size(recipe_dropdown, 150, 30); // Größe des Buttons anpassen
        lv_obj_align(recipe_dropdown, LV_ALIGN_TOP_MID, -70, 10);
        lv_obj_add_event_cb(recipe_dropdown, recipe_dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        dropdown_exists = true;
    }
}

void fileManagementFunction(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    preferences.begin("recipe_app", true); // "true" für lesezugriff
    int storedIndex = preferences.getInt("selectedRecipe", 0); // Standardwert ist -1, falls nichts gespeichert wurde
    preferences.end();
    if (storedIndex >= 0 && storedIndex < recipes.size()) {
        selectedRecipeIndex = storedIndex;
    }
    //clearContentArea();
    //createRecipeDropdown(content_container);
    //createSaveButton(content_container); // Button hinzufügen
    if (!recipes.empty()) {
        updateChartBasedOnRecipe(recipes[selectedRecipeIndex]);
    }
}