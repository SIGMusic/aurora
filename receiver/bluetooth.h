#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <inttypes.h>
#include "Arduino.h"

class BluetoothClass {
    public:
        /**
         * Begins the Bluetooth connection.
         * @param baudRate The baud rate to use
         * @param address The light address to broadcast
         */
        void begin(int baudRate, uint8_t address);

        /**
         * Ends the Bluetooth connection.
         */
        void end();

    private:
        /**
         * Sets the device name being broadcast.
         * @param channel The new channel number to broadcast
         */
        void setDeviceName(uint8_t channel);

        /**
         * Sets the PIN.
         * @param pin The new alphanumeric PIN
         */
        void setPIN(char pin[]);

        /**
         * Sets the baud rate.
         * @param baudRate The new baud rate to use
         * @note Supported baud rates: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
         */
        void setBaudRate(int baudRate);
};

extern BluetoothClass Bluetooth;

#endif