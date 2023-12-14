#include "SDCardHandler.h"
#include <memory>



SDCardHandler::SDCardHandler() : _isInitialized(false), spi(HSPI), db(nullptr) {}

std::string SDCardHandler::dbPathGlobal;

SDCardHandler::~SDCardHandler() {
        sqlite3_finalize(insertStmt); // Bereinigen des Prepared Statements

    if (db != nullptr) {
        sqlite3_close(db);
    }
}

void SDCardHandler::setDbPath(const std::string& path) {
    // Stellen Sie sicher, dass der Basisordner /sd existiert
    if (!SD.exists("/sd")) {
        SD.mkdir("/sd");
    }

    // Fügen Sie /sd/ zum Anfang des Pfades hinzu, falls nicht bereits vorhanden
    if (path.find("/sd") != 0) {
        dbPathGlobal = "/sd" + path;
    } else {
        dbPathGlobal = path;
    }
}

bool SDCardHandler::clearTable(const std::string& tableName) {
    // SQL-Befehl zum Leeren der Tabelle
    std::string deleteSql = "DELETE FROM " + tableName + ";";
    char* errMsg;

    // Ausführen des SQL-Befehls
    if (sqlite3_exec(db, deleteSql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        Serial.print("Fehler beim Leeren der Tabelle ");
        Serial.print(tableName.c_str());
        Serial.print(": ");
        Serial.println(errMsg);
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

bool SDCardHandler::createSetpointTable(const std::string& tableName) {
    // Leere die Tabelle, falls sie bereits existiert
    std::string deleteSql = "DELETE FROM " + tableName + ";";
    char* errMsg;
    if (sqlite3_exec(db, deleteSql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        Serial.println("Fehler beim Leeren der Setpoint-Tabelle.");
        sqlite3_free(errMsg);
        // Fahren Sie trotzdem fort, um die Tabelle zu erstellen
    }
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " (Timestamp INTEGER PRIMARY KEY, Temperature REAL);";
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        Serial.println("Fehler beim Erstellen der Setpoint-Tabelle.");
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool SDCardHandler::prepareInsertStatement(const std::string& tableName) {
    const char* sql = "INSERT OR REPLACE INTO ? (Day, Temperature) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &insertStmt, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(insertStmt, 1, tableName.c_str(), -1, SQLITE_STATIC);
    return true;
}

bool SDCardHandler::logSetpointData(int day, float temperature) {
    sqlite3_bind_int(insertStmt, 2, day);
    sqlite3_bind_double(insertStmt, 3, temperature);

    if (sqlite3_step(insertStmt) != SQLITE_DONE) {
        sqlite3_reset(insertStmt);
        return false;
    }
    sqlite3_reset(insertStmt);
    return true;
}

void SDCardHandler::beginTransaction() {
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
}

void SDCardHandler::endTransaction() {
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
}
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


bool SDCardHandler::openDatabase(const std::string& dbPath) {
    if (db != nullptr) {
        // Die Datenbank ist bereits geöffnet
        return true;
    }

    // Versuchen Sie, die Datenbank zu öffnen
    if (sqlite3_open("/sd/setpoint.db", &db) != SQLITE_OK) {
        Serial.println("Fehler beim Öffnen der SQLite-Datenbank.");
        return false;
    }

    // Tabelle erstellen, falls sie noch nicht existiert
    const char *createTableSQL = "CREATE TABLE IF NOT EXISTS TemperatureLog (Timestamp TEXT PRIMARY KEY, Temperature REAL);";
    char *errMsg;
    if (sqlite3_exec(db, createTableSQL, 0, 0, &errMsg) != SQLITE_OK) {
        Serial.println("Fehler beim Erstellen der Tabelle.");
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    return true;
}


    

