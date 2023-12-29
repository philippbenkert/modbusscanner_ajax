#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include "qrcodegen.h"
#include "ModbusSettings.h"
#include <Crypto.h>
#include <stdio.h>
#include <string.h>
#include "qrcodegen.h"


extern WebSocketHandler webSocketHandler;
extern bool loadCredentials(String& ssid, String& password);

void drawContent(String content) {
    clearContentArea();
    lv_obj_t *label = lv_label_create(content_container);
    lv_label_set_text(label, content.c_str());
    lv_obj_set_style_text_font(label, &lv_font_saira_500, 0);
    lv_obj_set_width(label, lv_obj_get_width(content_container));
    lv_obj_center(label);
}


// Funktion zum Entfernen von Doppelpunkten aus der MAC-Adresse
void remove_colons(char *dest, const char *src) {
    while (*src) {
        if (*src != ':') {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

// Funktion zum Erstellen eines SHA-256-Hash
void sha256_hash_string(const char *input, char *output) {
    SHA256 hasher;
    hasher.doUpdate(input, strlen(input));
    byte hash[SHA256_SIZE];
    hasher.doFinal(hash);

    for (int i = 0; i < SHA256_SIZE; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = 0;
}

void create_label(const char *text, lv_obj_t *parent, int x, int y) {
    lv_obj_t *label = lv_label_create(content_container);
    lv_label_set_text(label, text);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, y);
    // Hier können Sie zusätzliche Stiloptionen anwenden
}

void scanFunctionsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE);

        isMenuLocked = false;

        String mac_str = WiFi.macAddress();
        const char *mac_address = mac_str.c_str();
        char formatted_mac[18];
        remove_colons(formatted_mac, mac_address);

        char outputBuffer[65];
        sha256_hash_string(formatted_mac, outputBuffer);

        char url_output[50];
        snprintf(url_output, 50, "URL: http://179.43.154.10:8000/login", outputBuffer);
        create_label(url_output, obj, 10, 10);

        char id_output[50];
        snprintf(id_output, 50, "Geraete-ID: %s", formatted_mac);
        create_label(id_output, obj, 10, 40);

        char key_output[30];
        snprintf(key_output, 30, "Key: %.10s", outputBuffer);
        create_label(key_output, obj, 10, 70);

        // Erstellen des QR-Codes
        char qr_data[100];
        snprintf(qr_data, 100, "http://179.43.154.10:8000/login?device_id=%s&password=%.10s", formatted_mac, outputBuffer);
        lv_obj_t *qrcode = lv_qrcode_create(content_container, 150, lv_color_black(), lv_color_white());

        lv_obj_align(qrcode, LV_ALIGN_CENTER, 50, 60);
        lv_qrcode_update(qrcode, qr_data, strlen(qr_data));
    }
}