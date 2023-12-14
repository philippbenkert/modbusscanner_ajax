#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include <string> // Dieses Header-File einbinden
#include <vector>
#include "Recipe.h"

struct TimeTempPair {
    unsigned long time;
    float temperature;
};

std::vector<TimeTempPair> readDatabaseData(const char* dbName, const std::string& tableName);
// Weitere Funktionserklärungen

#endif // DATABASEHANDLER_H
