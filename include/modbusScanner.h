#include "ModbusClientRTU.h"
#include <HardwareSerial.h>

const uint8_t BOARD_485_EN = 2;
String getModbusConfig(const char* key);

class ModbusScanner {
public:
    ModbusScanner(HardwareSerial& serial, int8_t rtsPin = BOARD_485_EN);  // Hier setzen wir den Standardwert
    void begin();
    String scanRegisters();
    String scanFunction(uint16_t address, uint8_t function);
    String scanSpecificRegister(uint16_t regAddress);
    bool isClientReachable();
private:
    ModbusClientRTU node;
    uint8_t deviceAddress;
};