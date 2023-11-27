#include "ModbusClientRTU.h"
#include <HardwareSerial.h>
#include <vector> // Für die Verwendung von std::vector

const uint8_t BOARD_485_EN = 2;


// Struktur für Modbus-Gerätekonfiguration
struct ModbusDeviceConfig {
    uint16_t istTemperaturReg;
    uint16_t sollTemperaturReg;
    uint16_t sollMinimumReg;
    uint16_t sollMaximumReg;
    String geraetName;  // Hinzugefügtes Feld für den Reglernamen
    int versuche = 0; // Hinzugefügt, um die Anzahl der Versuche zu zählen

};

std::vector<ModbusDeviceConfig> readModbusConfigs(const char* filename);

class ModbusScanner {
public:
    ModbusScanner(HardwareSerial& serial, int8_t rtsPin = BOARD_485_EN);
    void begin();
    bool identifyDevices(std::vector<ModbusDeviceConfig>& configs); // Geänderte Methode
    bool isClientReachable();
    bool tryConnectAndIdentify(std::vector<ModbusDeviceConfig>& configs);
private:
    ModbusClientRTU node;
    uint8_t deviceAddress;
};
