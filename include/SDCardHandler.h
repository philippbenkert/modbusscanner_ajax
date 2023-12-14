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
    static std::string dbPath; // Pfad zur Datenbank
    bool _isInitialized;
    SPIClass spi;
    sqlite3 *db; // SQLite3-Datenbankobjekt

    static std::string dbPathGlobal;
public:
    SDCardHandler();
    ~SDCardHandler();
    bool init();
    bool mkdir(const char* path);
    File open(const char* path, const char* mode);
    bool openDatabase(const std::string& dbPath);
    bool logData(const char* timestamp, float temperature);
    static void setDbPath(const std::string& path);
    static const std::string& getDbPath() {
        return dbPathGlobal;
    }
    bool createSetpointTable(const std::string& tableName);
    bool logSetpointData(const std::string& tableName, int day, float temperature);
    sqlite3* getDb() const {
        return db;
    }
    bool clearTable(const std::string& tableName);
};