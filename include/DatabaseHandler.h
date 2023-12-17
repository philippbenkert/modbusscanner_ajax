#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include <string> // Dieses Header-File einbinden
#include <vector>
#include "Recipe.h"

struct TimeTempPair {
    unsigned long time;
    int temperature;
    int logtemp;
};

std::vector<TimeTempPair> readDatabaseData(const char* dbName, const std::string& tableName);
// Weitere Funktionserklärungen
void writeSingleDataPoint(const std::string& tableName);
void exportDataToXML(const char* dbName, const std::string& tableName, unsigned long startCoolingTime);


#endif // DATABASEHANDLER_H
