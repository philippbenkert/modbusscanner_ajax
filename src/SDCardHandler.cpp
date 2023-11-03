#include "SDCardHandler.h"

SDCardHandler::SDCardHandler() : _isInitialized(false), spi(HSPI), db(nullptr) {
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

bool SDCardHandler::mkdir(const char* path) {
    String dummyFilePath = String(path) + "/dummy.txt";
    File dummyFile = open(dummyFilePath.c_str(), FILE_WRITE);
    if (dummyFile) {
        dummyFile.close();
        SD.remove(dummyFilePath.c_str());
        return true;
    }
    return false;
}


File SDCardHandler::open(const char* path, const char* mode) {
    File file = SD.open(path, mode);
    if (file) {
    } else {
    }
    return file;
}

bool SDCardHandler::openDatabase(const char* dbPath) {
    if (sqlite3_open(dbPath, &db) != SQLITE_OK) {
        return false;
    }
    return true;
}

bool SDCardHandler::executeSQL(const char* sql) {
    char* errMsg;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
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
