/**
 * SIGMusic Lights 2015
 * Arduino receiver firmware
 */

#include "Arduino.h"
#include "bluetooth.h"

#define HEADER_LENGTH       4
#define DATA_LENGTH         3
#define CHECKSUM_LENGTH     1
#define PACKET_LENGTH       (HEADER_LENGTH + DATA_LENGTH + CHECKSUM_LENGTH)

#define HEADER_INDEX        0
#define DATA_INDEX          (HEADER_LENGTH)
#define CHECKSUM_INDEX      (HEADER_LENGTH + DATA_LENGTH)

//The pins that each switch is attached to
#define SWITCH_0            2
#define SWITCH_1            3
#define SWITCH_2            4
#define SWITCH_3            5
#define SWITCH_4            6
#define SWITCH_5            7
#define SWITCH_6            8
#define SWITCH_7            9

const uint8_t validHeader[] = {'S','I','G','M'};
uint8_t myChannel = 0xFF; //The address of the current light, set by DIP switch

void setup() {
    //Initialize the built-in LED to output (for debugging)
    pinMode(LED_BUILTIN, OUTPUT);

    initSwitches();
    myChannel = readChannel();
    Bluetooth.begin(19200, myChannel);
    Serial.print("Channel ");
    Serial.println(myChannel);
}

void loop() {
    static int currentIndex = 0; //The packet index of the data we've received
    static uint8_t tempRGB[DATA_LENGTH] = {0};

    //Wait for incoming data
    if (Serial.available()) {
        uint8_t receivedByte = Serial.read();

        if (currentIndex < HEADER_LENGTH) {
            //We haven't yet received a valid header
            if (receivedByte == validHeader[currentIndex]) {
                //The received byte matches the header. Move to the next index.
                currentIndex++;
            } else {
                //The received byte doesn't match. Restart the state machine.
                currentIndex = 0;
                Serial.println("ERR");
            }
        } else {
            //We have received a valid header
             if (currentIndex < CHECKSUM_INDEX) {
                //Store the temporary RGB values until we validate the checksum
                tempRGB[currentIndex - DATA_INDEX] = receivedByte;
                currentIndex++;
             } else {
                //Validate the checksum
                if (calculateChecksum(tempRGB) == receivedByte) {
                    /* It's valid. Set the new RGB values and restart the state
                     * machine.
                     */
                    setRGB(tempRGB[0], tempRGB[1], tempRGB[2]);
                    currentIndex = 0;
                    Serial.println("OK");
                } else {
                    //The checksum is invalid. Restart the state machine.
                    currentIndex = 0;
                    Serial.println("ERR");
                }
             }
        }
    }
}

/**
 * Sets up the DIP switches as inputs with internal pullup resistors.
 */
void initSwitches() {
    pinMode(SWITCH_0, INPUT_PULLUP);
    pinMode(SWITCH_1, INPUT_PULLUP);
    pinMode(SWITCH_2, INPUT_PULLUP);
    pinMode(SWITCH_3, INPUT_PULLUP);
    pinMode(SWITCH_4, INPUT_PULLUP);
    pinMode(SWITCH_5, INPUT_PULLUP);
    pinMode(SWITCH_6, INPUT_PULLUP);
    pinMode(SWITCH_7, INPUT_PULLUP);
}

/**
 * Updates this node's channel from the DIP switches.
 * @return The channel read from the DIP switches
 */
uint8_t readChannel() {
    /* Read the switch values. Since we're using the internal pullup resistor,
     * the bits are inverted. A HIGH signal means the switch is open.
     */
    int bit0 = (digitalRead(SWITCH_0) == HIGH ? 0 : 1);
    int bit1 = (digitalRead(SWITCH_1) == HIGH ? 0 : 1);
    int bit2 = (digitalRead(SWITCH_2) == HIGH ? 0 : 1);
    int bit3 = (digitalRead(SWITCH_3) == HIGH ? 0 : 1);
    int bit4 = (digitalRead(SWITCH_4) == HIGH ? 0 : 1);
    int bit5 = (digitalRead(SWITCH_5) == HIGH ? 0 : 1);
    int bit6 = (digitalRead(SWITCH_6) == HIGH ? 0 : 1);
    int bit7 = (digitalRead(SWITCH_7) == HIGH ? 0 : 1);

    //Build the channel from the bits
    uint8_t myChannel = bit0;
    myChannel |= bit1 << 1;
    myChannel |= bit2 << 2;
    myChannel |= bit3 << 3;
    myChannel |= bit4 << 4;
    myChannel |= bit5 << 5;
    myChannel |= bit6 << 6;
    myChannel |= bit7 << 7;

    return myChannel;
}

/**
 * Calculates the expected checksum of a received packet.
 * @param data The received data (RGB only)
 * @return The 8-bit checksum of the data
 */
uint8_t calculateChecksum(uint8_t data[]) {
    uint8_t checksum = 0;
    for (int i = 0; i < DATA_LENGTH; i++) {
        checksum += data[i]; //Relies on 8-bit integer overflow
    }
    return checksum;
}

/**
 * Sets the RGB values to be output from the light.
 * @param red The new red value
 * @param green The new green value
 * @param blue The new blue value
 */
void setRGB(uint8_t red, uint8_t green, uint8_t blue) {
    /*
     * TODO: make this actually set the RGB values
     */
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
}