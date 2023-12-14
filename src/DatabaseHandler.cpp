#include "DatabaseHandler.h"
#include "SDCardHandler.h"

extern SDCardHandler sdCard;

std::vector<TimeTempPair> readDatabaseData(const char* dbName, const std::string& tableName) {
    std::vector<TimeTempPair> data;
    sqlite3_stmt *stmt;

    Serial.println("Lesen der Datenbank gestartet");

    // Öffnen der Datenbank
    if (!sdCard.openDatabase(dbName)) {
        Serial.println("Failed to open database");
        return data;
    }

    // Vorbereiten der SQL-Abfrage
    std::string sql = "SELECT Timestamp, Temperature FROM " + tableName + ";";
    if (sqlite3_prepare_v2(sdCard.getDb(), sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        Serial.print("Fehler beim Vorbereiten der Abfrage: ");
        Serial.println(sqlite3_errmsg(sdCard.getDb()));  // Gibt eine detaillierte Fehlermeldung aus
        return data;
    }

    // Ausführen der Abfrage und Lesen der Daten
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TimeTempPair pair;

        // Abrufen des Zeitstempels als Text und Konvertierung in eine long-Variable
        const char* timeText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        pair.time = strtoul(timeText, NULL, 10); // Konvertierung von Text zu unsigned long

        // Abrufen der Temperatur als Gleitkommazahl
        pair.temperature = sqlite3_column_double(stmt, 1);

        data.push_back(pair);
    }

    // Bereinigen
    sqlite3_finalize(stmt);

    return data;
}

// Implementierungen anderer Datenbank-bezogener Funktionen
