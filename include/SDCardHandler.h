// SDCardHandler.h
#pragma once

#include <SD.h>
#include <SPI.h>
#include <SQLite3.h>

const uint8_t SD_MISO = GPIO_NUM_38;
const uint8_t SD_MOSI = GPIO_NUM_40;
const uint8_t SD_SCLK = GPIO_NUM_39;
const uint8_t SD_CS = GPIO_NUM_41;

class SDCardHandler {
private:
    bool _isInitialized;
    SPIClass spi;
    sqlite3* db;
public:
    SDCardHandler();
    bool init();
    File open(const char* path, const char* mode);
    bool isInitialized() const { return _isInitialized; }
    
    // SQLite-Operationen
    bool openDatabase(const char* dbPath);
    bool executeSQL(const char* sql);
    // ... Weitere SQLite-bezogene Methoden ...
    ~SDCardHandler();
};
