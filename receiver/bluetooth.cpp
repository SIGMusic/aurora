#include "bluetooth.h"

void BluetoothClass::begin(int baudRate, uint8_t address) {
    Serial.begin(baudRate);
    setBaudRate(baudRate);
    setDeviceName(address);
}

void BluetoothClass::setDeviceName(uint8_t channel) {
    char newName[] = {0};
    sprintf(newName, "Light%3%0%d", channel);
    Serial.print("AT+NAME");
    Serial.print(newName);
}

void BluetoothClass::setPIN(char pin[]) {
    Serial.print("AT+PIN");
    Serial.print(pin);
}

void BluetoothClass::setBaudRate(int baudRate) {
    switch (baudRate) {
        case 1200:
            Serial.print("AT+BAUD1");
            break;
        case 2400:
            Serial.print("AT+BAUD2");
            break;
        case 4800:
            Serial.print("AT+BAUD3");
            break;
        case 9600:
            Serial.print("AT+BAUD4");
            break;
        case 19200:
            Serial.print("AT+BAUD5");
            break;
        case 38400:
            Serial.print("AT+BAUD6");
            break;
        case 57600:
            Serial.print("AT+BAUD7");
            break;
        case 115200:
            Serial.print("AT+BAUD8");
            break;
        //There are higher baud rates, but not that Arduino supports.
        default:
            Serial.println("ERROR: Baud rate unsupported");
            return;
    }

    Serial.end();
    Serial.begin(baudRate);
}

BluetoothClass Bluetooth; //Create a Bluetooth object for the user