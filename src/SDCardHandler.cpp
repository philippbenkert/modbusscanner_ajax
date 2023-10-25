// SDCardHandler.cpp
#include "SDCardHandler.h"

SDCardHandler::SDCardHandler() : _isInitialized(false), spi(HSPI) {  // Verwenden Sie HSPI oder VSPI je nach Bedarf
}

bool SDCardHandler::init() {
    
    spi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);  // Initialisieren Sie SPI mit Ihren Pins
    if (!SD.begin(SD_CS, spi)) {
        _isInitialized = false;
        return false;
    }
    _isInitialized = true;
    return true;
}

File SDCardHandler::open(const char* path, const char* mode) {
    return SD.open(path, mode);
}

