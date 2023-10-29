#include "SDCardHandler.h"

SDCardHandler::SDCardHandler() : _isInitialized(false), spi(HSPI) {  // Verwenden Sie HSPI oder VSPI je nach Bedarf
}

bool SDCardHandler::init() {
    
    spi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);  // Initialisieren Sie SPI mit Ihren Pins
    if (!SD.begin(SD_CS, spi)) {
        Serial.println("SDCardHandler: SD-Karte konnte nicht initialisiert werden!");
        _isInitialized = false;
        return false;
    }
    Serial.println("SDCardHandler: SD-Karte erfolgreich initialisiert.");
    _isInitialized = true;
    return true;
}

File SDCardHandler::open(const char* path, const char* mode) {
    File file = SD.open(path, mode);
    if (file) {
        Serial.printf("SDCardHandler: Datei '%s' erfolgreich geöffnet.\n", path);
    } else {
        Serial.printf("SDCardHandler: Fehler beim Öffnen der Datei '%s'.\n", path);
    }
    return file;
}
