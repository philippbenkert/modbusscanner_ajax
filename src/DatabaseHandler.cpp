#include "DatabaseHandler.h"
#include "SDCardHandler.h"
#include "DateTimeHandler.h"
#include <XMLWriter.h>

extern SDCardHandler sdCard;

std::vector<TimeTempPair> readDatabaseData(const std::string& dbName, const std::string& tableName) {
    std::vector<TimeTempPair> data;
    std::string sql = "SELECT Timestamp, round(Temperature, 2), logTemp FROM " + tableName + ";";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(sdCard.getDb(), sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            data.emplace_back(TimeTempPair{
                strtoul(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)), nullptr, 10),
                sqlite3_column_int(stmt, 1),
                sqlite3_column_int(stmt, 2)
            });
        }
        sqlite3_finalize(stmt);
    } else {
        Serial.print("Fehler beim Vorbereiten der Abfrage: ");
        Serial.println(sqlite3_errmsg(sdCard.getDb()));
    }
    return data;
}

void writeSingleDataPoint(const std::string& tableName, int modbusLogTemp, unsigned long currentTime) {
    if (sdCard.prepareInsertStatement(tableName)) {
        sdCard.beginTransaction();
        int dataValue = modbusLogTemp == 0 ? 99999 : modbusLogTemp;
        if (!sdCard.logTemperatureData(currentTime, dataValue)) {
            Serial.println("Error logging data to database");
        }
        sdCard.endTransaction();
    } else {
        Serial.println("Failed to prepare insert statement");
    }
}

void exportDataToXML(const std::string& dbName, const std::string& tableName, unsigned long startCoolingTime) {
    std::vector<TimeTempPair> data = readDatabaseData(dbName, tableName);
    std::string filename = "/sd/" + std::to_string(startCoolingTime) + ".xml";
    File file = sdCard.open(filename.c_str(), FILE_WRITE);

    if (file) {
        XMLWriter xml(&file);
        xml.header();
        xml.tagOpen("Data");

        for (const auto& pair : data) {
            if (pair.time > 1700000000) {
                xml.tagOpen("DataPoint");
                xml.writeNode("Timestamp", String(pair.time).c_str());
                xml.writeNode("LogTemp", String(pair.logtemp).c_str());
                xml.tagClose("");
            }
        }
        xml.tagClose("Data");
        file.close();
    } else {
        Serial.println("Error opening file for XML export");
    }
}