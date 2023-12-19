#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include <string>
#include <vector>
#include "Recipe.h"

struct TimeTempPair {
    unsigned long time;
    int temperature;
    int logtemp;
};

std::vector<TimeTempPair> readDatabaseData(const std::string& dbName, const std::string& tableName);
void writeSingleDataPoint(const std::string& tableName, int modbusLogTemp, unsigned long currentTime);
void exportDataToXML(const std::string& dbName, const std::string& tableName, unsigned long startCoolingTime);

#endif // DATABASEHANDLER_H
