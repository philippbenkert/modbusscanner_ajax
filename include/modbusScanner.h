#include <ModbusMaster.h>
#include <HardwareSerial.h>


class ModbusScanner {
public:
    ModbusScanner(); // Ändern Sie die Konstruktorsignatur entsprechend.
    void begin();
    String scanRegisters();
    String scanFunction(uint16_t address, uint8_t function);
    String scanSpecificRegister(uint16_t regAddress);
    bool isClientReachable();
private:
    ModbusMaster node;
    uint8_t deviceAddress;
};