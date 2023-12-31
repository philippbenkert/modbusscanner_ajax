#ifndef RS232NTCSensor_h
#define RS232NTCSensor_h

#include <Arduino.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

class RS232NTCSensor {
public:
    RS232NTCSensor(int8_t rxPin, int8_t txPin);
    bool begin();
    float readTemperature();
    bool isModuleConnected();

private:
    ModbusMaster modbus;
    SoftwareSerial modbusSerial;
    int8_t rxPin, txPin;
    bool isConnected;
};

#endif
