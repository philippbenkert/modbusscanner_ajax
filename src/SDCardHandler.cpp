#include "SDCardHandler.h"

SDCardHandler::SDCardHandler() : _isInitialized(false), spi(HSPI), db(nullptr) {
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

bool SDCardHandler::openDatabase(const char* dbPath) {
    if (sqlite3_open(dbPath, &db) != SQLITE_OK) {
        Serial.println("Fehler beim Öffnen der SQLite-Datenbank!");
        return false;
    }
    return true;
}

bool SDCardHandler::executeSQL(const char* sql) {
    char* errMsg;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        Serial.printf("SQLite-Fehler: %s\n", errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

SDCardHandler::~SDCardHandler() {
    if (db) {
        sqlite3_close(db);
    }
}
