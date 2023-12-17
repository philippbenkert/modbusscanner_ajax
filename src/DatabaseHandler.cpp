#include "DatabaseHandler.h"
#include "SDCardHandler.h"
#include "DateTimeHandler.h"
#include <XMLWriter.h>

extern unsigned long startCoolingTime;
extern SDCardHandler sdCard;
extern RTC_DS3231 rtc;
std::vector<TimeTempPair> readDatabaseData(const char* dbName, const std::string& tableName) {
    std::vector<TimeTempPair> data;

    if (!sdCard.openDatabase(dbName)) {
        Serial.println("Failed to open database");
        return data;
    }

    // Rundet die Temperatur auf zwei Nachkommastellen
    std::string sql = "SELECT Timestamp, round(Temperature, 2), logTemp FROM " + tableName + ";";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(sdCard.getDb(), sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        Serial.print("Fehler beim Vorbereiten der Abfrage: ");
        Serial.println(sqlite3_errmsg(sdCard.getDb()));
        return data;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TimeTempPair pair = {
            .time = strtoul(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), NULL, 10),
            .temperature = sqlite3_column_int(stmt, 1), // Direktes Auslesen als Ganzzahl
            .logtemp = sqlite3_column_int(stmt, 2) // Direktes Auslesen als Ganzzahl
        };
        data.push_back(pair);
    }

    sqlite3_finalize(stmt);
    return data;
}

extern int modbusLogTemp; // Ihre globale Variable

void writeSingleDataPoint(const std::string& tableName) {
    if (!sdCard.openDatabase("/sd/setpoint.db")) {
        Serial.println("Failed to open database");
        return;
    }

    if (!sdCard.prepareInsertStatement(tableName)) {
        Serial.println("Failed to prepare insert statement");
        return;
    }

    // Ermitteln des aktuellen Zeitstempels
    DateTime now = rtc.now();
    unsigned long currentTime = now.unixtime();

    sdCard.beginTransaction();

    int dataValue = (modbusLogTemp == 0) ? 99999 : modbusLogTemp; // Überprüfen, ob globalIntValue "leer" ist

    if (!sdCard.logTemperatureData(currentTime, dataValue)) {
        Serial.println("Error logging data to database");
    }

    sdCard.endTransaction();
}

void exportDataToXML(const char* dbName, const std::string& tableName, unsigned long startCoolingTime) {
    std::vector<TimeTempPair> data = readDatabaseData(dbName, tableName);

    // Erstelle die Datei für das Schreiben der XML-Daten
    std::string filename = "/sd/" + std::to_string(startCoolingTime) + ".xml";
    File file = sdCard.open(filename.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("Error opening file for XML export");
        return;
    }

    // Initialisiere den XMLWriter mit der Datei als Ausgabestream
    XMLWriter xml(&file);

    // Starte das Dokument
    xml.header();

    // Beginne das Root-Element
    xml.tagOpen("Data");

    for (const auto& pair : data) {
        if (pair.time > 1700000000) {
            xml.tagOpen("DataPoint");
            
            // Konvertiere `unsigned long` zu `String` und füge es als Node hinzu
            xml.writeNode("Timestamp", String(pair.time).c_str());
            xml.writeNode("LogTemp", String(pair.logtemp).c_str());

            xml.tagClose();
        }
    }

    // Beende das Root-Element
    xml.tagClose();

    // Schließe die Datei
    file.close();
}


