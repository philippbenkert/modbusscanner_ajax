// SDCardHandler.h
#pragma once

#include <SD.h>  // Verwenden Sie die SD-Bibliothek für SPI
#include <SPI.h>

// SD-Karten-Pins definieren
const uint8_t SD_MISO = GPIO_NUM_38;
const uint8_t SD_MOSI = GPIO_NUM_40;
const uint8_t SD_SCLK = GPIO_NUM_39;
const uint8_t SD_CS = GPIO_NUM_41;

class SDCardHandler {
private:
    bool _isInitialized;
    SPIClass spi;
public:
    SDCardHandler();
    bool init();
    File open(const char* path, const char* mode);
    bool isInitialized() const { return _isInitialized; }
    // Weitere Methoden und Funktionen für die SD-Kartenverwaltung...
};
