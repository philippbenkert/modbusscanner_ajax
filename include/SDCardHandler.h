// SDCardHandler.h
#pragma once

#include <SD.h>
#include <SPI.h>
#include <ulog_sqlite.h>

const uint8_t SD_MISO = GPIO_NUM_38;
const uint8_t SD_MOSI = GPIO_NUM_40;
const uint8_t SD_SCLK = GPIO_NUM_39;
const uint8_t SD_CS = GPIO_NUM_41;

class SDCardHandler {
private:
    static String dbPath; // Pfad zur Datenbank
    bool _isInitialized;
    SPIClass spi;
    dblog_write_context wctx;
    dblog_read_context rctx;
    static std::string dbPathGlobal;

public:
    SDCardHandler();
    ~SDCardHandler();
    bool init();
    bool mkdir(const char* path);
    File open(const char* path, const char* mode);
    bool openDatabase(const char* dbPath);
    bool logData(const char* data);
    static int32_t readData(dblog_write_context *ctx, void *buf, uint32_t pos, size_t len);
    static int32_t writeData(dblog_write_context *ctx, void *buf, uint32_t pos, size_t len);
    static void setDbPath(const std::string& path);

};