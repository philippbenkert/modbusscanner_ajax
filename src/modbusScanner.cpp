#include "modbusScanner.h"
#include "LittleFS.h"
#include "ArduinoJson.h"
#include <esp_exception.h>  // Sie benötigen diese Header-Datei
#include <esp_task_wdt.h>
#include <Preferences.h>
#include "ModbusSettings.h"


#define BOARD_485_TX                42
#define BOARD_485_RX                1

extern Preferences preferences;

const uint16_t MODBUS_MAX_BUFFER = 128;  // Hinzugefügt
uint16_t responseBuffer[MODBUS_MAX_BUFFER];

ModbusScanner modbusScanner(Serial2);

std::vector<ModbusDeviceConfig> readModbusConfigs(const char* filename) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return {};
    }

    DynamicJsonDocument doc(2048); // Größe anpassen, um alle Konfigurationen zu erfassen
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to parse config file");
        return {};
    }

    std::vector<ModbusDeviceConfig> configs;
    for (JsonObject obj : doc.as<JsonArray>()) {
        ModbusDeviceConfig config;
        config.geraetName = obj["geraetName"] | "Unbekanntes Gerät";
        config.istTemperaturReg = obj["istTemperaturReg"];
        config.sollTemperaturReg = obj["sollTemperaturReg"];
        config.sollMinimumReg = obj["sollMinimumReg"];
        config.sollMaximumReg = obj["sollMaximumReg"];
        configs.push_back(config);
    }

    return configs;
}

bool ModbusScanner::identifyDevices(std::vector<ModbusDeviceConfig>& configs) {
    bool deviceIdentified = false;
    String identifiedDeviceName = "Kein Gerät";

    for (auto& config : configs) {
        if (config.versuche < 1) {
            uint8_t serverID = deviceAddress;
            uint8_t functionCode = READ_HOLD_REGISTER;
            uint16_t startAddress = config.istTemperaturReg;
            uint16_t registerCount = 1;
            uint32_t token = 0; // You might need to provide a meaningful token here

            Error res = node.addRequest(token, serverID, functionCode, startAddress, registerCount);

            if (res == SUCCESS) {
    unsigned long startTime = millis();
    while (millis() - startTime < 100) {
        ModbusMessage response = node.syncRequest(token, serverID, functionCode, startAddress, registerCount);

        // Check if the response is valid and not an error
        if (response.getFunctionCode() == functionCode && response.getError() == SUCCESS) {
            deviceIdentified = true;
            identifiedDeviceName = config.geraetName;
            break;
        }
    }
} else {
    config.versuche++;
}

            if (deviceIdentified) {
                break;
            }
        }
    }

    return deviceIdentified;
}



ModbusScanner::ModbusScanner(HardwareSerial& serial, int8_t rtsPin) : node(rtsPin) {
    preferences.begin("modbus_settings", true);
    deviceAddress = preferences.getUInt("deviceAddress", 1); // Standardwert 1
    preferences.end();
}

bool ModbusScanner::isClientReachable() {
    Error res = node.addRequest(0, deviceAddress, READ_HOLD_REGISTER, 0, 1);
    if (res == SUCCESS) {
        return true;
    } else {
        ModbusError me(res);
        Serial.printf("Error: %02X - %s\n", res, (const char *)me);
        return false;
    }
}

bool ModbusScanner::tryConnectAndIdentify(std::vector<ModbusDeviceConfig>& configs) {
    if (isClientReachable()) {
        return identifyDevices(configs);
    }
    return false; // Verbindung fehlgeschlagen oder Gerät nicht identifiziert
}

void ModbusScanner::begin() {
    preferences.begin("modbus_settings", true);
    uint32_t baudRate = preferences.getUInt("baudrate", 19200); // Standardwert 19200
    String parity = preferences.getString("parity", "n"); // Standardwert "n"
    uint8_t stopBits = preferences.getUChar("stopbits", 2); // Standardwert 1

    // Konfigurieren Sie Serial2 basierend auf den gespeicherten Einstellungen.
    if (parity == "n" && stopBits == 1) {
        Serial2.begin(baudRate, SERIAL_8N1, BOARD_485_RX, BOARD_485_TX);
    } else if (parity == "n" && stopBits == 2) {
        Serial2.begin(baudRate, SERIAL_8N2, BOARD_485_RX, BOARD_485_TX);
    } else if (parity == "e" && stopBits == 1) {
        Serial2.begin(baudRate, SERIAL_8E1, BOARD_485_RX, BOARD_485_TX);
    } else if (parity == "e" && stopBits == 2) {
        Serial2.begin(baudRate, SERIAL_8E2, BOARD_485_RX, BOARD_485_TX);
    } else {
        // Standardfall: Sie können zusätzliche Bedingungen hinzufügen, falls Sie andere Formate unterstützen möchten.
        Serial2.begin(baudRate, SERIAL_8N1, BOARD_485_RX, BOARD_485_TX); // Standardformat mit RX und TX Pins
    }

    // Echo-Test für Serial2
    Serial2.println("Echo Test");
    delay(100);  // Warten Sie kurz, um sicherzustellen, dass die Daten gesendet wurden

    if (Serial2.available()) {
        String receivedData = Serial2.readString();
        if (receivedData.startsWith("Echo Test")) {
        } else {
        }
    } else {
    }
    node.begin(Serial2);  // Verwenden Sie hier Serial2
        preferences.end();

}