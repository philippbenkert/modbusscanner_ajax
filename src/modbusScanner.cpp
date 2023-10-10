#include "modbusScanner.h"
#include "LittleFS.h"
#include "ArduinoJson.h"
#include <esp_exception.h>  // Sie benötigen diese Header-Datei


const uint8_t BOARD_485_EN = 42;
const size_t JSON_DOC_SIZE = 256;
const char* CONFIG_FILE = "/modbus-config.json";

ModbusScanner modbusScanner;

void sendModbusRequest() {
    digitalWrite(BOARD_485_EN, HIGH);
    delayMicroseconds(100);  // Verwenden von delayMicroseconds für kürzere Verzögerungen
}

void receiveModbusResponse() {
    digitalWrite(BOARD_485_EN, LOW);
    delayMicroseconds(100);
}

bool saveModbusConfig(const char* key, const char* value) {
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    File file = LittleFS.open(CONFIG_FILE, "r+");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to read JSON");
        file.close();
        return false;
    }

    doc[key] = value;
    file.seek(0);
    serializeJson(doc, file);
    file.close();
    return true;
}

String getModbusConfig(const char* key) {
    DynamicJsonDocument doc(JSON_DOC_SIZE);
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return String("");
    }

    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to read JSON");
        file.close();
        return String("");
    }
    file.close();

    if (doc.containsKey(key)) {
        return doc[key].as<String>();
    } else {
        return String("");
    }
}

ModbusScanner::ModbusScanner() : node() {
    Serial.println("Initializing ModbusScanner...");
    deviceAddress = getModbusConfig("deviceAddress").toInt();
    if(deviceAddress == 0) deviceAddress = 1;
    Serial.println("ModbusScanner initialized.");
}


bool ModbusScanner::isClientReachable() {
    Serial.println("Checking client reachability...");
    sendModbusRequest();
    uint8_t res = node.readInputRegisters(0, 1);
    receiveModbusResponse();
    Serial.println(res == node.ku8MBSuccess ? "Client is reachable." : "Client is not reachable.");
    return (res == node.ku8MBSuccess);
}


void ModbusScanner::begin() {
    Serial.println("Scanning registers...");

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
    Serial.println("Finished scanning registers.");

}


String ModbusScanner::scanRegisters() {
    String result = "Function,Address,Value\n";

    if (modbusScanner.isClientReachable()) {
    // Führen Sie Ihre Modbus-Operationen hier aus


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
    } else {
    Serial.println("Modbus client is not reachable!");
    }
    return result;

}



String ModbusScanner::scanFunction(uint16_t address, uint8_t function) {
    Serial.print("Scanning function: ");
    Serial.print(function);
    Serial.print(", Address: ");
    Serial.println(address);
    
    String result = "";
    uint8_t res = 0;

    try {
        sendModbusRequest();  // Starten Sie den Sendemodus

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

        receiveModbusResponse();  // Wechseln Sie in den Empfangsmodus

        if (res == node.ku8MBSuccess) {
            result = "Function " + String(function) + "," + String(address) + "," + String(node.getResponseBuffer(0)) + "\n";
        } else {
            result = "Function " + String(function) + "," + String(address) + ",No response\n";
        }
    } catch (const char* msg) {
        Serial.print("Exception caught: ");
        Serial.println(msg);
        result = "Function " + String(function) + "," + String(address) + ",Exception caught\n";
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
