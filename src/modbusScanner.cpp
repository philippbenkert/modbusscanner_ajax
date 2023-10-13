#include "modbusScanner.h"
#include "LittleFS.h"
#include "ArduinoJson.h"
#include <esp_exception.h>  // Sie benötigen diese Header-Datei


const size_t JSON_DOC_SIZE = 256;
const char* CONFIG_FILE = "/config/modbus-config.json";
const uint16_t MODBUS_MAX_BUFFER = 128;  // Hinzugefügt
uint16_t responseBuffer[MODBUS_MAX_BUFFER];


ModbusScanner modbusScanner(Serial2);
HardwareSerial& mySerial2 = Serial2;

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

ModbusScanner::ModbusScanner(HardwareSerial& serial, int8_t rtsPin) : node(rtsPin) {
    Serial.println("Initializing ModbusScanner...");
    deviceAddress = getModbusConfig("deviceAddress").toInt();
    if(deviceAddress == 0) deviceAddress = 1;
    Serial.println("ModbusScanner initialized.");
}



bool ModbusScanner::isClientReachable() {
    Serial.println("Checking client reachability...");
    Error res = node.addRequest(0, deviceAddress, READ_HOLD_REGISTER, 0, 1);
    if (res == SUCCESS) {
        return true;
    } else {
        ModbusError me(res);
        Serial.printf("Error: %02X - %s\n", res, (const char *)me);
        return false;
    }
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
        mySerial2.begin(baudRate, SERIAL_8N1);
    } else if (parity == "n" && stopBits == 2) {
        mySerial2.begin(baudRate, SERIAL_8N2);
    } else if (parity == "e" && stopBits == 1) {
        mySerial2.begin(baudRate, SERIAL_8E1);
    } else if (parity == "e" && stopBits == 2) {
        mySerial2.begin(baudRate, SERIAL_8E2);
    } else {
        // Standardfall: Sie können zusätzliche Bedingungen hinzufügen, falls Sie andere Formate unterstützen möchten.
        mySerial2.begin(baudRate);
    }

    node.begin(mySerial2);
    Serial.println("Finished scanning registers.");
}


String ModbusScanner::scanRegisters() {
    String result = "Function,Address,Value\n";

    if (modbusScanner.isClientReachable()) {
        // Führen Sie Ihre Modbus-Operationen hier aus
        for (uint16_t i = 0; i <= 3000; i++) {
            // readCoils
            result += scanFunction(i, 1);
            delay(50);  // Pause von 50ms

            // readDiscreteInputs
            result += scanFunction(i, 2);
            delay(50);  // Pause von 50ms

            // readHoldingRegisters
            result += scanFunction(i, 3);
            delay(50);  // Pause von 50ms

            // readInputRegisters
            result += scanFunction(i, 4);
            delay(50);  // Pause von 50ms
        }
        return result;
    } else {
        Serial.println("Modbus client is not reachable!");
    }
    return result;
}


void handleData(ModbusMessage msg, uint32_t token) 
{
    Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", msg.getServerID(), msg.getFunctionCode(), token, msg.size());
    int i = 0;
    for (auto& byte : msg) {
        responseBuffer[i++] = byte;
        Serial.printf("%02X ", byte);
    }
    Serial.println("");
}


String ModbusScanner::scanFunction(uint16_t address, uint8_t function) {
    Serial.print("Scanning function: ");
    Serial.print(function);
    Serial.print(", Address: ");
    Serial.println(address);
    
    String result = "";
    uint8_t fc = 0;  // Funktion Code

    switch (function) {
        case 1:
            fc = READ_COIL;
            break;
        case 2:
            fc = READ_DISCR_INPUT;
            break;
        case 3:
            fc = READ_HOLD_REGISTER;
            break;
        case 4:
            fc = READ_INPUT_REGISTER;
            break;
    }

    Error res = node.addRequest(0, deviceAddress, fc, address, 1);

    if (res == SUCCESS) {
    result = "Function " + String(function) + "," + String(address) + "," + String(responseBuffer[0]) + "\n";
    } else {
        ModbusError me(res);
        result = "Function " + String(function) + "," + String(address) + ",Error: " + String((const char *)me) + "\n";
    }

    return result;
}

String ModbusScanner::scanSpecificRegister(uint16_t regAddress) {
    String result = "";
    Error res = node.addRequest(0, deviceAddress, READ_INPUT_REGISTER, regAddress, 1);

    if (res == SUCCESS) {
        result = String(responseBuffer[0]);  // Korrigiert
    } else {
        ModbusError me(res);
        result = "Error: " + String((const char *)me);
    }
    return result;
}