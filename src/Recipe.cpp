#include "Recipe.h"
#include "TouchPanel.h"
#include "Process.h"
#include "WLANSettings.h"
#include "DateTimeHandler.h"
#include <Preferences.h>
#include <memory>
#include "DatabaseHandler.h"
#include "ChartHandler.h"
#include "CommonDefinitions.h"

const int Y_LABEL_OFFSET = 30;
extern std::vector<Recipe> recipes;
std::vector<lv_obj_t*> x_axis_labels;
std::vector<lv_obj_t*> temp_labels;
lv_obj_t* cursor_info_label;
extern lv_obj_t* recipe_dropdown;
lv_style_t style;
static lv_style_t axis_style; // Stil für Achsenbeschriftungen
extern bool coolingProcessRunning;
extern DateTime now;
extern std::vector<Recipe> recipes;
extern Preferences preferences;
String currentDb;

// Funktion, um den Fortschritt zu berechnen
int calculateCoolingProgress(unsigned long currentTime, unsigned long startCoolingTime, const Recipe& recipe) {
  int daysProgressed; // Deklaration von daysProgressed
  daysProgressed = currentTime - startCoolingTime; // Initialisierung 
  daysProgressed = daysProgressed / 86400; // 86400 Sekunden pro Tag
  // Begrenze die Anzahl der Tage auf die Länge des Rezepts
  if (daysProgressed >= recipe.temperatures.size()) {
    daysProgressed = recipe.temperatures.size(); 
  }
  return daysProgressed; // Rückgabe des Ergebnisses
}

void updateProgress() {
    // Lesen des aktuellen Datenbanknamens aus den Preferences
    
    // Lesen der Datenbank-Daten
    dbData = readDatabaseData("setpoint.db", "Setpoints");
    // Aktuelle Zeit ermitteln
    unsigned long currentTime = now.unixtime();
    // Aktualisieren des Fortschrittscharts
    updateProgressChart(chart, progress_ser, dbData, startTime);
    writeSingleDataPoint("Setpoints");
}

lv_obj_t* createLabel(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t y) {
    lv_obj_t* label = lv_label_create(parent);
    if (!label) return nullptr;
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}

void initAxisStyle() {
    if (!initAxisStyle) {
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_make(255, 0, 0)); // Setzt die Textfarbe auf Schwarz
    }
}

void readRecipesFromFile() {
    File file = LittleFS.open("/kurven.csv", "r");
    if (!file) {
        return;
    }
    String line;
    line.reserve(128); // Reserviere genug Speicher für die Zeile, um häufige Allokationen zu vermeiden
    file.readStringUntil('\n'); // Überspringe die Kopfzeile
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
            continue; // Überspringe leere Zeilen
        }
        Recipe recipe;
        int start = 0;
        int end = line.indexOf(';');
        recipe.name = line.substring(start, end); // Extrahiere den Rezeptnamen
        start = end + 1;
        while ((end = line.indexOf(';', start)) != -1) {
            recipe.temperatures.push_back(line.substring(start, end).toInt()); // Extrahiere und konvertiere die Temperatur
            start = end + 1;
        }
        if (start < line.length()) {
            recipe.temperatures.push_back(line.substring(start).toInt()); // Verarbeite das letzte Stück der Zeile
        }
        recipes.push_back(std::move(recipe)); // Verwende std::move, um Kopien zu vermeiden
    }
    file.close();
}

const Recipe& getCurrentRecipe() {
    uint16_t selected_id = lv_dropdown_get_selected(recipe_dropdown);
    if (selected_id < recipes.size()) {
        return recipes[selected_id];
    } else {
        static Recipe default_recipe;
        return default_recipe;
    }
}



void updateSeriesColor(lv_obj_t* chart, lv_color_t color) {
    if (ser && chart && lv_obj_is_valid(chart)) {
        lv_chart_set_series_color(chart, ser, color);
    }
}

lv_obj_t* createChart(lv_obj_t* parent, lv_chart_type_t chart_type, const Recipe& recipe, lv_style_t* style) {
    lv_obj_t* chart = lv_chart_create(parent);
    lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -5);
    lv_chart_set_type(chart, chart_type);
    lv_obj_add_style(chart, &style_no_border, 0);

    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end()) - Y_AXIS_PADDING;
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end()) + Y_AXIS_PADDING;
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp *1000, max_temp *1000);
    return chart;
}