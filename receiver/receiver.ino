/**
 * SIGMusic Lights 2015
 * Arduino receiver firmware
 */

#include "Arduino.h"

#define HEADER_LENGTH       4
#define DATA_LENGTH         3
#define CHECKSUM_LENGTH     1
#define PACKET_LENGTH       (HEADER_LENGTH + DATA_LENGTH + CHECKSUM_LENGTH)

#define HEADER_INDEX        0
#define DATA_INDEX          (HEADER_LENGTH)
#define CHECKSUM_INDEX      (HEADER_LENGTH + DATA_LENGTH)

//RGB pins
#define RED_PIN             9
#define GREEN_PIN           10
#define BLUE_PIN            11

const uint8_t validHeader[HEADER_LENGTH] = {'S','I','G','M'};
uint8_t myChannel = 0xFF; //The address of the current light, set by DIP switch

void setup() {
    Serial.begin(19200);
    Serial.println("Ready");
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
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);

    //Debug
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
}