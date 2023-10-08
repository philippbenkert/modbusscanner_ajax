#include "modbusScanner.h"
#include "LittleFS.h"

ModbusScanner modbusScanner;


void saveModbusConfig(const char* key, const char* value) {
    File file = LittleFS.open("/modbus-config.txt", "a");  // Öffnet die Datei im Anhängemodus
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    file.println(String(key) + "=" + String(value));
    file.close();
}

String getModbusConfig(const char* key) {
    File file = LittleFS.open("/modbus-config.txt", "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return String("");
    }

    String line;
    while (file.available()) {
        line = file.readStringUntil('\n');
        if (line.startsWith(key)) {
            file.close();
            return line.substring(String(key).length() + 1);
        }
    }
    file.close();
    return String("");
}

ModbusScanner::ModbusScanner() : node() {
    deviceAddress = getModbusConfig("deviceAddress").toInt();
    if(deviceAddress == 0) deviceAddress = 1;  // Standardwert
}

void ModbusScanner::begin() {
    uint32_t baudRate = getModbusConfig("baudrate").toInt();
    if(baudRate == 0) baudRate = 19200;  // Standardwert
    
    String parity = getModbusConfig("parity");
    if(parity == "") parity = "n";  // Standardwert
    
    uint8_t stopBits = getModbusConfig("stopbits").toInt();
    if(stopBits == 0) stopBits = 1;  // Standardwert

    // Konfigurieren Sie Serial2 basierend auf den gespeicherten Einstellungen.
    if (parity == "n" && stopBits == 1) {
        Serial2.begin(baudRate, SERIAL_8N1);
    } else if (parity == "n" && stopBits == 2) {
        Serial2.begin(baudRate, SERIAL_8N2);
    } else if (parity == "e" && stopBits == 1) {
        Serial2.begin(baudRate, SERIAL_8E1);
    } else if (parity == "e" && stopBits == 2) {
        Serial2.begin(baudRate, SERIAL_8E2);
    } else {
        // Standardfall: Sie können zusätzliche Bedingungen hinzufügen, falls Sie andere Formate unterstützen möchten.
        Serial2.begin(baudRate);
    }

    node.begin(deviceAddress, Serial2);
}


String ModbusScanner::scanRegisters() {
    String result = "Function,Address,Value\n";

    for (uint16_t i = 0; i <= 3000; i++) {
        // readCoils
        result += scanFunction(i, 1);
        // readDiscreteInputs
        result += scanFunction(i, 2);
        // readHoldingRegisters
        result += scanFunction(i, 3);
        // readInputRegisters
        result += scanFunction(i, 4);
    }
    return result;
}

String ModbusScanner::scanFunction(uint16_t address, uint8_t function) {
    String result = "";
    uint8_t res = 0;
    switch (function) {
        case 1:
            res = node.readCoils(address, 1);
            break;
        case 2:
            res = node.readDiscreteInputs(address, 1);
            break;
        case 3:
            res = node.readHoldingRegisters(address, 1);
            break;
        case 4:
            res = node.readInputRegisters(address, 1);
            break;
    }

    if (res == node.ku8MBSuccess) {
        result = "Function " + String(function) + "," + String(address) + "," + String(node.getResponseBuffer(0)) + "\n";
    } else {
        result = "Function " + String(function) + "," + String(address) + ",No response\n";
    }

    return result;
}

String ModbusScanner::scanSpecificRegister(uint16_t regAddress) {
    // (Dieser Teil bleibt unverändert, da er nur für eine spezifische Adresse ist)
    String result = "";
    uint8_t res = node.readInputRegisters(regAddress, 1);  // Ein Register lesen

    if (res == node.ku8MBSuccess) {
        result = String(node.getResponseBuffer(0));
    } else {
        result = "No response";
    }
    return result;
}
