#include "RS232NTCSensor.h"

RS232NTCSensor::RS232NTCSensor(int8_t rxPin, int8_t txPin) : modbusSerial(rxPin, txPin), isConnected(false) {
    this->rxPin = rxPin;
    this->txPin = txPin;
}

bool RS232NTCSensor::begin() {
    modbusSerial.begin(9600);
    modbus.begin(1, modbusSerial); // Slave ID 1

    // Fügen Sie hier die Logik ein, um zu überprüfen, ob das Modul verbunden ist
    // Zum Beispiel, senden Sie eine Testanfrage und überprüfen Sie die Antwort
    isConnected = isModuleConnected();
    return isConnected;
}

float RS232NTCSensor::readTemperature() {
    if (!isConnected) {
        return -999.99; // Port nicht geöffnet oder Modul nicht verbunden
    }

    uint8_t result = modbus.readInputRegisters(0x0000, 1); // Registeradresse und -anzahl
    if (result == modbus.ku8MBSuccess) {
        return modbus.getResponseBuffer(0) / 10.0; // Angenommene Datenformatierung
    }
    return -999.99; // Fehlerwert
}

bool RS232NTCSensor::isModuleConnected() {
    uint8_t result = modbus.readInputRegisters(0x0000, 1); // Registeradresse und -anzahl
    if (result == modbus.ku8MBSuccess) {
    return false; // Angenommen, es ist verbunden
    } else  {return true;
            }
}
