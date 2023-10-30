#include "ContentDrawer.h"
#include "TouchPanel.h"
#include "MenuDrawer.h"
#include <qrcodegen.h>

extern WebSocketHandler webSocketHandler;
extern bool loadCredentials(String& ssid, String& password);

void wlanSettingsFunction(lv_event_t * e) {
    Serial.println("wlanSettingsFunction: Start");
    lv_obj_t * obj = lv_event_get_target(e);
    
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        Serial.println("Button clicked");
        clearContentArea();
        
        String wifiSSID;
        String wifiPassword;

        if (loadCredentials(wifiSSID, wifiPassword)) {
            Serial.println("Loaded credentials successfully");

            char qrText[256];
            snprintf(qrText, sizeof(qrText), "WIFI:T:WPA;S:%s;P:%s;H:false;;", wifiSSID.c_str(), wifiPassword.c_str());
            
            uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
            uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
            
            Serial.println("Attempting to generate QR code");
            bool success = qrcodegen_encodeText(qrText, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
                                                qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

            if (success) {
                Serial.println("QR code generation successful");
                
                int size = qrcodegen_getSize(qrcode);
                Serial.println("Attempting to allocate memory for QR code image");
                lv_color_t *buf = (lv_color_t *)malloc(size * size * sizeof(lv_color_t));

                if (buf == NULL) {
                    Serial.println("Memory allocation for buffer failed!");
                    return;
                }
                
                Serial.println("Drawing QR code to buffer");
                for(int y = 0; y < size; y++) {
                    for(int x = 0; x < size; x++) {
                        buf[y * size + x] = qrcodegen_getModule(qrcode, x, y) ? lv_color_make(0, 0, 0) : lv_color_make(255, 255, 255);
                    }
                }

                lv_img_dsc_t qr_img_dsc;  
                qr_img_dsc.data = (uint8_t *)buf;
                qr_img_dsc.header.w = size;
                qr_img_dsc.header.h = size;
                qr_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;

                Serial.println("Displaying QR code on screen");
                lv_obj_t * img = lv_img_create(content_container);
                lv_img_set_src(img, &qr_img_dsc);
                lv_obj_set_size(img, 200, 200);  
                lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

                free(buf);  // Freigabe des Speichers

            } else {
                Serial.println("QR code generation failed");
                drawContent("Fehler beim Generieren des QR-Codes für WLAN-Einstellungen.");
            }

        } else {
            Serial.println("Failed to load credentials");
            drawContent("Fehler beim Laden der WLAN-Einstellungen.");
        }
    }
    Serial.println("wlanSettingsFunction: End");
}

void drawContent(String content) {
    
    lv_obj_t *label = lv_label_create(content_container);
    lv_label_set_text(label, content.c_str());
    lv_obj_set_style_text_font(label, &lv_font_saira_500, 0);  // Setzen Sie die Schriftart für das Label
    lv_obj_center(label);
}

void modbusSettingsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Modbus Einstellungen Inhalt...");
    }
}

void fileManagementFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Datei-Management Inhalt...");
    }
}

void scanFunctionsFunction(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        clearContentArea();
        drawContent("Scan-Funktionen Inhalt...");
    }
}