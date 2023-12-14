#include "Recipe.h"
#include "TouchPanel.h"
#include "SDCardHandler.h" // If needed for file operations
#include "FileManagement.h"
#include "WLANSettings.h"
#include "DateTimeHandler.h"
#include <Preferences.h>
#include "ulog_sqlite.h"   // Bibliothek für die Datenbanklogik
#include <memory>

extern SDCardHandler sdCard;
static lv_chart_cursor_t* cursor = nullptr; // Globale oder statische Variable für den Cursor
const int Y_AXIS_PADDING = 2;
const int Y_LABEL_OFFSET = 30;
extern std::vector<Recipe> recipes;
std::vector<lv_obj_t*> x_axis_labels;
std::vector<lv_obj_t*> temp_labels;
lv_obj_t* cursor_info_label;
extern lv_obj_t* recipe_dropdown;
static lv_style_t style;
static lv_style_t axis_style; // Stil für Achsenbeschriftungen
extern bool coolingProcessRunning;
extern RTC_DS3231 rtc;
extern std::vector<Recipe> recipes;
extern Preferences preferences;
String currentDb;

void clearCursor();

// Struktur zur Speicherung der Zeit-/Temperaturdaten
struct TimeTempPair {
    unsigned long time;
    float temperature;
};

// Funktion, um den Fortschritt zu berechnen
int calculateCoolingProgress(unsigned long currentTime, unsigned long startCoolingTime, const Recipe& recipe) {
    unsigned long elapsedSeconds = currentTime - startCoolingTime;
    int daysProgressed = elapsedSeconds / 86400; // 86400 Sekunden pro Tag

    // Begrenze die Anzahl der Tage auf die Länge des Rezepts
    if (daysProgressed >= recipe.temperatures.size()) {
        daysProgressed = recipe.temperatures.size() - 1;
    }

    return daysProgressed;
}
std::vector<TimeTempPair> readDatabaseData(const char* dbName, const std::string& tableName) {
    std::vector<TimeTempPair> data;
    sqlite3_stmt *stmt;

    Serial.println("Lesen der Datenbank gestartet");

    // Öffnen der Datenbank
    if (!sdCard.openDatabase(dbName)) {
        Serial.println("Failed to open database");
        return data;
    }

    // Vorbereiten der SQL-Abfrage
    std::string sql = "SELECT Timestamp, Temperature FROM " + tableName + ";";
    if (sqlite3_prepare_v2(sdCard.getDb(), sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        Serial.print("Fehler beim Vorbereiten der Abfrage: ");
        Serial.println(sqlite3_errmsg(sdCard.getDb()));  // Gibt eine detaillierte Fehlermeldung aus
        return data;
    }

    // Ausführen der Abfrage und Lesen der Daten
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TimeTempPair pair;

        // Abrufen des Zeitstempels als Text und Konvertierung in eine long-Variable
        const char* timeText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        pair.time = strtoul(timeText, NULL, 10); // Konvertierung von Text zu unsigned long

        // Abrufen der Temperatur als Gleitkommazahl
        pair.temperature = sqlite3_column_double(stmt, 1);

        data.push_back(pair);
    }

    // Bereinigen
    sqlite3_finalize(stmt);

    return data;
}



// Funktion, um den Fortschrittschart zu aktualisieren
void updateProgressChart(lv_obj_t* chart, const std::vector<TimeTempPair>& data, unsigned long currentTime) {
    if (!chart || !progress_ser) {
        Serial.println("Chart oder Series nicht initialisiert.");
        return;
    }
    // Setze die Punkteanzahl auf die Gesamtanzahl der Datenpunkte
    lv_chart_set_point_count(chart, data.size());
    // Durchlaufe alle Datenpunkte und füge sie dem Chart hinzu
    for (size_t i = 0; i < data.size(); ++i) {
        const auto& dataPoint = data[i];
        if (dataPoint.time <= currentTime) {
            // Gültiger Datenpunkt
            lv_chart_set_next_value(chart, progress_ser, dataPoint.temperature);
        } else {
            // Setze leeren Wert für Punkte nach der aktuellen Zeit
            lv_chart_set_next_value(chart, progress_ser, LV_CHART_POINT_NONE);
        }
    }
}

void updateProgress() {
    Serial.println("Updatevorgang gestartet.");
    // Lesen des aktuellen Datenbanknamens aus den Preferences
    
    // Lesen der Datenbank-Daten
    std::vector<TimeTempPair> dbData = readDatabaseData(currentDb.c_str(), "TemperatureLog");

    // Aktuelle Zeit ermitteln
    unsigned long currentTime = rtc.now().unixtime();
    Serial.println(currentTime);

    // Aktualisieren des Fortschrittscharts
    Serial.println("UpdateChart gestartet.");
    updateProgressChart(chart, dbData, currentTime);
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

void updateCursorInfo(lv_obj_t* chart, const Recipe& recipe, uint16_t point_idx) {
    // Berechne den Tagesindex basierend auf dem Punktindex
    uint16_t dayIndex = point_idx / 12; 

    // Sicherstellen, dass der Tagesindex im Bereich der Rezeptlänge liegt
    if (dayIndex >= recipe.temperatures.size()) {
        return; 
    }

    // Holen Sie sich die Temperatur für den berechneten Tag
    int temp = recipe.temperatures[dayIndex];
    char buf[64];
    snprintf(buf, sizeof(buf), "Tag %d: %d°C", dayIndex, temp); // "+1", weil Tage normalerweise ab 1 gezählt werden
    lv_label_set_text(cursor_info_label, buf);
    lv_obj_clear_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN); // Zeige das Label an
}


void updateSeriesColor(lv_obj_t* chart, lv_color_t color) {
    if (ser && chart && lv_obj_is_valid(chart)) {
        lv_chart_set_series_color(chart, ser, color);
    }
}

void chart_touch_event_cb(lv_event_t* e) {
    if (coolingProcessRunning) {
        return;
    }
    chart = lv_event_get_target(e);
    if (!chart || !lv_obj_is_valid(chart)) {
        return; // Beenden, wenn das Chart-Objekt ungültig ist
    }
    if (!cursor) {
        return; // Beenden, wenn der Cursor ungültig ist
    }
    const Recipe& current_recipe = getCurrentRecipe(); // Verwenden Sie die neue Funktion
    uint32_t point_id = lv_chart_get_pressed_point(chart);
    if (point_id == LV_CHART_POINT_NONE) {
        return; // Beenden, wenn kein Punkt gedrückt wurde
    }
    lv_point_t p_out;
    lv_chart_get_point_pos_by_id(chart, ser, point_id, &p_out);

    if (point_id != LV_CHART_POINT_NONE) {
    lv_chart_set_cursor_point(chart, cursor, ser, point_id);
    }    
    updateCursorInfo(chart, current_recipe, point_id);
    if(chart && lv_obj_has_class(chart, &lv_chart_class)) {
    lv_chart_refresh(chart);
}
}

void updateCursorVisibility(lv_obj_t* chart, bool visible) {
        if (visible) {
            if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
            lv_obj_clear_flag(recipe_dropdown, LV_OBJ_FLAG_HIDDEN); // Macht das Dropdown wieder sichtbar
            }
                       
        } else {
            if (cursor) {
            }
            Serial.println("Versteckt das Dropdown."); // Debug-Ausgabe
            if (recipe_dropdown && lv_obj_is_valid(recipe_dropdown)) {
            lv_obj_add_flag(recipe_dropdown, LV_OBJ_FLAG_HIDDEN); // Versteckt das Dropdown
            } 
        }


}


void clearCursor() {
    if (cursor) {
        //lv_obj_del((lv_obj_t*)cursor);
        cursor = nullptr;
        if(chart && lv_obj_has_class(chart, &lv_chart_class)) {
        lv_chart_refresh(chart);
        }
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
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp, max_temp);

    if (chart_type == LV_CHART_TYPE_LINE) {
        lv_chart_set_point_count(chart, recipe.temperatures.size() * 12); // 12 Punkte pro Tag
    }

    lv_obj_add_style(chart, style, 0);
    return chart;
}

void create_chart(){
    static bool is_chart_style_initialized = false;
    static lv_style_t chart_style;

    chart = lv_chart_create(content_container);
    lv_obj_add_style(chart, &style_no_border, 0);
    lv_obj_set_size(chart, TFT_WIDTH - 60, 170);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 10, -10);
    cursor_info_label = lv_label_create(chart);
    lv_label_set_text(cursor_info_label, "");
    lv_obj_set_size(cursor_info_label, 100, 20);
    lv_obj_align(cursor_info_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_flag(cursor_info_label, LV_OBJ_FLAG_HIDDEN);

    // Initialisiere den Stil nur, wenn es das erste Mal ist
    if (!is_chart_style_initialized) {
        lv_style_init(&chart_style);
        lv_style_set_size(&chart_style, 3);
        is_chart_style_initialized = true;
    }

    if (chart && lv_obj_is_valid(chart)) {
        lv_obj_add_style(chart, &chart_style, LV_PART_INDICATOR);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
        progress_ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    }
}

float interpolate(int dayIndex, int subIndex, const Recipe& recipe) {
    if (dayIndex >= recipe.temperatures.size() - 1) {
        return recipe.temperatures.back();
    }
    float startTemp = recipe.temperatures[dayIndex];
    float endTemp = recipe.temperatures[dayIndex + 1];
    float step = (endTemp - startTemp) / 12.0; // 12 Schritte pro Tag
    return startTemp + step * subIndex;
}

void updateChartBasedOnRecipe(const Recipe& recipe) {
    clearContentArea();
    SDCardHandler::setDbPath("/setpoint.db");
    Serial.println("Aktuelle Datenbank: /setpoint.db");

    lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);
    createRecipeDropdown(content_container);
    createToggleCoolingButton(content_container);

    // Lese Daten aus der Datenbank
    std::vector<TimeTempPair> dbData = readDatabaseData("/setpoint.db", "Setpoints");

    int totalPoints = dbData.size();
    int min_temp = *std::min_element(recipe.temperatures.begin(), recipe.temperatures.end());
    int max_temp = *std::max_element(recipe.temperatures.begin(), recipe.temperatures.end());

    if (!chart || !lv_obj_is_valid(chart)) {
        create_chart();
    }

    if (chart && lv_obj_is_valid(chart) && lv_obj_has_class(chart, &lv_chart_class)) {
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, min_temp - Y_AXIS_PADDING, max_temp + Y_AXIS_PADDING);
        lv_chart_set_point_count(chart, totalPoints);

        clearCursor();
        if (!cursor) {
            cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_VER);
        }

        initAxisStyle();
        // Anpassung der X-Achsenbeschriftung entsprechend der Anzahl der Datenpunkte
        int numLabels = dbData.size() /12; 
        if (dbData.size() % 12 != 0) {
        numLabels += 1;
        }
        lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, numLabels, 1, true, 20);
        int y_major_tick_count = 10; 
        lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 0, y_major_tick_count, 1, true, 25);
        lv_obj_add_event_cb(chart, chart_touch_event_cb, LV_EVENT_PRESSED, nullptr);

        lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
        // Setze die Temperaturen aus der Datenbank
        for (int i = 0; i < dbData.size(); ++i) {
            lv_chart_set_next_value(chart, ser, dbData[i].temperature);
        }

        lv_obj_add_style(chart, &style, LV_PART_TICKS);
        lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
        lv_chart_refresh(chart);
    }

    if (coolingProcessRunning) {
        displayEndTime(savedEndTime);
    } else {
        coolingProcessRunning = false;
        displayEndTime(savedEndTime);
    }
    updateToggleCoolingButtonText();
}


void clearLabels(std::vector<lv_obj_t*>& labels) {
    for (auto& label : labels) {
        if (label && lv_obj_is_valid(label)) {
            lv_obj_del(label);
            label = nullptr;
        }
    }
    labels.clear();
}