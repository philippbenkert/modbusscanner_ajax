// CommonDefinitions.h
#ifndef COMMONDEFINITIONS_H
#define COMMONDEFINITIONS_H

#include "DatabaseHandler.h"
#include <vector>
#include <Arduino.h>


bool loadCredentials(String& savedSSID, String& savedPassword);

const int Y_AXIS_PADDING = 2;
extern std::vector<TimeTempPair> dbData;

#endif // COMMONDEFINITIONS_H
