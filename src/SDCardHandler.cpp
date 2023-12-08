#include "SDCardHandler.h"
#include <memory>



SDCardHandler::SDCardHandler() : _isInitialized(false), spi(HSPI), wctx({0}), rctx({0}) {}

std::string SDCardHandler::dbPathGlobal;

bool SDCardHandler::init() {
    
    spi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);  // Initialisieren Sie SPI mit Ihren Pins
    Serial.println("Initialisiere SD-Karte.");
    if (!SD.begin(SD_CS, spi)) {
        Serial.println("Fehler: SD-Karte konnte nicht initialisiert werden.");
        _isInitialized = false;
        return false;
    }
    Serial.println("SD-Karte erfolgreich initialisiert.");
    _isInitialized = true;
    return true;
}

void SDCardHandler::setDbPath(const std::string& path) {
    dbPathGlobal = path;
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
    return SD.open(path, mode);
}


bool SDCardHandler::openDatabase(const char* dbPath) {
    Serial.println("Öffnen der Datenbank gestartet.");

    dbPathGlobal = dbPath;
    Serial.print("Datenbankpfad gesetzt: ");
    Serial.println(dbPathGlobal.c_str());

    // Verwenden von std::unique_ptr für automatische Speicherverwaltung
    std::unique_ptr<byte[]> writeBuffer(new byte[4096]);
    wctx.buf = writeBuffer.get();
    wctx.page_size_exp = 9;

    wctx.read_fn = &SDCardHandler::readData;
    wctx.write_fn = &SDCardHandler::writeData;
    wctx.flush_fn = [](struct dblog_write_context *ctx) -> int { return 0; };

    Serial.println("Initialisiere Datenbankschreibkontext.");
    if (dblog_write_init(&wctx) != DBLOG_RES_OK) {
        Serial.println("Fehler bei der Initialisierung des Schreibkontexts.");
        return false;
    }

    std::unique_ptr<byte[]> readBuffer(new byte[4096]);
    rctx.buf = readBuffer.get();
    rctx.page_size_exp = wctx.page_size_exp;
    rctx.read_fn = reinterpret_cast<int32_t(*)(dblog_read_context *, void *, uint32_t, size_t)>(wctx.read_fn);

    Serial.println("Initialisiere Datenbanklesekontext.");
    if (dblog_read_init(&rctx) != DBLOG_RES_OK) {
        Serial.println("Fehler bei der Initialisierung des Lesekontexts.");
        return false;
    }

    Serial.println("Datenbank erfolgreich geöffnet.");
    return true;
}

bool SDCardHandler::logData(const char* data) {
    Serial.println("Beginne logData-Funktion");

    const void* values[1] = {data};
    uint8_t types[1] = {DBLOG_TYPE_TEXT};
    uint16_t lengths[1] = {static_cast<uint16_t>(strlen(data))};

    Serial.print("Einzufügende Daten: ");
    Serial.println(data);

    int res = dblog_append_row_with_values(&wctx, types, values, lengths);
    if (res != DBLOG_RES_OK) {
        Serial.print("Fehler beim Einfügen in die Datenbank. Fehlercode: ");
        Serial.println(res);
        return false;
    }

    Serial.println("Daten erfolgreich in die Datenbank eingefügt");

    res = dblog_flush(&wctx);
    if (res != DBLOG_RES_OK) {
        Serial.print("Fehler beim Spülen der Datenbank. Fehlercode: ");
        Serial.println(res);
        return false;
    }

    Serial.println("Datenbank erfolgreich gespült");
    return true;
}


SDCardHandler::~SDCardHandler() {
    dblog_finalize(&wctx);
    delete[] wctx.buf;
    delete[] rctx.buf;
}

int32_t SDCardHandler::readData(dblog_write_context *ctx, void *buf, uint32_t pos, size_t len) {
    Serial.println("Lesevorgang gestartet.");

    File file = SD.open(dbPathGlobal.c_str(), FILE_READ);
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Lesen.");
        return -1;
    }

    file.seek(pos);
    int32_t bytesRead = file.read(reinterpret_cast<byte*>(buf), len);
    file.close();

    Serial.println("Lesevorgang abgeschlossen.");
    return bytesRead;
}

int32_t SDCardHandler::writeData(dblog_write_context *ctx, void *buf, uint32_t pos, size_t len) {
    Serial.println("Schreibvorgang gestartet.");

    // Überprüfen Sie, ob die Datei existiert. Wenn nicht, erstellen Sie sie.
    if (!SD.exists(dbPathGlobal.c_str())) {
        File file = SD.open(dbPathGlobal.c_str(), FILE_WRITE);
        if (file) {
            Serial.println("Datei erstellt.");
            file.close();
        } else {
            Serial.println("Fehler beim Erstellen der Datei.");
            return -1;
        }
    }

    // Versuchen Sie nun, die Datei zum Schreiben zu öffnen.
    File file = SD.open(dbPathGlobal.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("Fehler beim Öffnen der Datei zum Schreiben.");
        return -1;
    }

    file.seek(pos);
    int32_t bytesWritten = file.write(reinterpret_cast<const byte*>(buf), len);
    file.close();

    Serial.println("Schreibvorgang abgeschlossen.");
    return bytesWritten;
}
