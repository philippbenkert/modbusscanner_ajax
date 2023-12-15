#include "DatabaseHandler.h"
#include "SDCardHandler.h"

extern SDCardHandler sdCard;

std::vector<TimeTempPair> readDatabaseData(const char* dbName, const std::string& tableName) {
    std::vector<TimeTempPair> data;

    Serial.println("Lesen der Datenbank gestartet");

    if (!sdCard.openDatabase(dbName)) {
        Serial.println("Failed to open database");
        return data;
    }

    std::string sql = "SELECT Timestamp, Temperature FROM " + tableName + ";";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(sdCard.getDb(), sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        Serial.print("Fehler beim Vorbereiten der Abfrage: ");
        Serial.println(sqlite3_errmsg(sdCard.getDb()));
        return data;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TimeTempPair pair = {
            .time = strtoul(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), NULL, 10),
            .temperature = sqlite3_column_double(stmt, 1)
        };
        data.push_back(pair);
    }

    sqlite3_finalize(stmt);
    return data;
}