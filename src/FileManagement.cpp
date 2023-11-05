#include "FileManagement.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "SDCardHandler.h"

#define DATA_POINT_LABEL 1  // Eindeutiger Identifikator für Datenpunkt-Labels


// Definitionen und Implementierungen
lv_obj_t * slider1;
lv_obj_t * slider2;
lv_obj_t * chart;
lv_chart_series_t * ser;
lv_obj_t * value_label1;
lv_obj_t * value_label2;

extern SDCardHandler sdCard;

#define MAX_DAYS 6  // Maximale Anzahl von Tagen, die Sie erwarten
lv_obj_t* data_labels[MAX_DAYS] = { NULL };  // Globales Array zum Speichern von Datenpunkt-Labels

void slider_event_cb(lv_event_t *e) {
    if (!value_label1 || !value_label2) {
        return;
    }
    if (!slider1 || !slider2) {
        return;
    }
    int days = lv_slider_get_value(slider1);
    int temp = lv_slider_get_value(slider2);
    // Labels aktualisieren
    lv_label_set_text_fmt(value_label1, "%d Tage", days);
    lv_label_set_text_fmt(value_label2, "%d°C", temp);
    // Chart aktualisieren
    updateChart(days, temp);
}

void deleteDataPointLabels() {
    for(int i = 0; i < MAX_DAYS; i++) {
        if(data_labels[i]) {
            lv_obj_del(data_labels[i]);
            data_labels[i] = NULL;
        }
    }
}

void updateChart(int days, int endTemp) {
    if (!ser) return;

    if (ser) {
        lv_chart_remove_series(chart, ser);
        ser = NULL;
        deleteDataPointLabels();
    }

    // Entfernen Sie alle vorherigen Datenpunkt-Labels
    // lv_obj_t *child = lv_obj_get_child(chart, 0);
    //while (child) {
    //    int user_data = (int)lv_obj_get_user_data(child);
    //    if (user_data == DATA_POINT_LABEL) {
    //        lv_obj_t *next_child = lv_obj_get_child(chart, lv_obj_get_index(child) + 1);
    //        lv_obj_del(child);
    //        child = next_child;
    //    } else {
    //        child = lv_obj_get_child(chart, lv_obj_get_index(child) + 1);
    //    }
    //}

    lv_color_t red_color = LV_COLOR_MAKE(255, 0, 0);
    ser = lv_chart_add_series(chart, red_color, LV_CHART_AXIS_PRIMARY_Y);
    if (!ser) return;

    lv_chart_set_point_count(chart, days + 1);
    lv_chart_set_next_value(chart, ser, 20); // Starttemperatur immer +20°C

    for (int i = 1; i <= days; i++) {
        // Calculate the temperature
        int curr_temp = 20 + ((endTemp - 20) * i / days); // If this causes truncation errors, consider using floating point division and rounding.
        lv_chart_set_next_value(chart, ser, curr_temp);

        // Create and position the label
        lv_obj_t *point_label = lv_label_create(chart);
        lv_label_set_text_fmt(point_label, "%d°C", curr_temp);
        lv_obj_set_user_data(point_label, (void*)(intptr_t)DATA_POINT_LABEL);
        data_labels[i - 1] = point_label;

    // Positionieren Sie das Label basierend auf der X- und Y-Position des Datenpunkts
    int x_pos = (i * (TFT_WIDTH - 20) / days) - 10;  // -10, um das Label zu zentrieren
    int y_pos = (150 - (curr_temp + 20) * 150 / 40) - 20;  // Annahme: y-Bereich ist -20 bis 20 und -10px zusätzlich für die gewünschte Verschiebung
    lv_obj_set_pos(point_label, x_pos, y_pos);
    }
}



void fileManagementFunction(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    clearContentArea();
    // Slider 1 für Abkühldauer
    slider1 = lv_slider_create(content_container);
    if (!slider1) {
        return;
    }
    lv_obj_set_width(slider1, TFT_WIDTH - 80);
    lv_obj_center(slider1);
    lv_obj_align(slider1, LV_ALIGN_CENTER, 0, -100);
    lv_slider_set_range(slider1, 1, MAX_DAYS);
    lv_slider_set_value(slider1, 3, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider1, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // Label für Slider 1
    lv_obj_t *label_slider1 = lv_label_create(content_container);
    lv_label_set_text(label_slider1, "Abkühldauer:");
    lv_obj_align(label_slider1, LV_ALIGN_OUT_TOP_LEFT, 10, -10);
    // Wertanzeige für Slider 1
    value_label1 = lv_label_create(content_container);
    lv_obj_align(value_label1, LV_ALIGN_OUT_TOP_LEFT, 170, -10);
    // Slider 2 für Endtemperatur
    slider2 = lv_slider_create(content_container);
    if (!slider2) {
        return;
    }
    lv_obj_set_width(slider2, TFT_WIDTH - 80);
    lv_obj_center(slider2);
    lv_obj_align(slider2, LV_ALIGN_CENTER, 0, -30);
    lv_slider_set_range(slider2, -20, 20);
    lv_slider_set_value(slider2, -20, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider2, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // Label für Slider 2
    lv_obj_t *label_slider2 = lv_label_create(content_container);
    lv_label_set_text(label_slider2, "Endtemperatur:");
    lv_obj_align(label_slider2, LV_ALIGN_OUT_TOP_LEFT, 10, 60);
    // Wertanzeige für Slider 2
    value_label2 = lv_label_create(content_container);
    lv_obj_align(value_label2, LV_ALIGN_OUT_TOP_LEFT, 170, 60);
    // Chart
    chart = lv_chart_create(content_container);
    if (!chart) {
        return;
    }
    lv_obj_set_size(chart, TFT_WIDTH - 20, 150);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 1, 10);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -20, 20);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    if (!ser) {
        return;
    }
    lv_obj_set_scrollbar_mode(content_container, LV_SCROLLBAR_MODE_OFF); // Deaktivieren Sie die seitliche Scrollleiste
    // Initialwerte in die Wertanzeigen eintragen
    lv_label_set_text_fmt(value_label1, "%d Tage", lv_slider_get_value(slider1));
    lv_label_set_text_fmt(value_label2, "%d°C", lv_slider_get_value(slider2));


    updateChart(3, -20);

}
