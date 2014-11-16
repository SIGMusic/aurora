/**
 * SIGMusic Lights 2015
 * Arduino receiver firmware
 */

#define HEADER_LENGTH       6
#define DATA_LENGTH         3
#define CHECKSUM_LENGTH     1
#define PACKET_LENGTH       (HEADER_LENGTH + DATA_LENGTH + CHECKSUM_LENGTH)

#define HEADER_INDEX        0
#define DATA_INDEX          (HEADER_LENGTH)
#define CHECKSUM_INDEX      (HEADER_LENGTH + DATA_LENGTH)

const uint8_t validHeader[] = {'S','I','G','M','U','S'};

void setup() {
    //Initialize serial over Bluetooth
    Serial.begin(9600);

    //Initialize the built-in LED to output (for debugging)
    pinMode(LED_BUILTIN, OUTPUT);
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
                Serial.print("Received valid header byte ");
                Serial.print(receivedByte);
                Serial.print(" at index ");
                Serial.println(currentIndex);
                //The received byte matches the header. Move to the next index.
                currentIndex++;
            } else {
                Serial.print("Received INVALID header byte ");
                Serial.print(receivedByte);
                Serial.print(" at index ");
                Serial.println(currentIndex);
                //The received byte doesn't match. Restart the state machine.
                currentIndex = 0;
            }
        } else {
            //We have received a valid header
             if (currentIndex < CHECKSUM_INDEX) {
                Serial.print("Received data byte ");
                Serial.print(receivedByte, HEX);
                Serial.print(" at index ");
                Serial.println(currentIndex);
                //Store the temporary RGB values until we validate the checksum
                tempRGB[currentIndex - DATA_INDEX] = receivedByte;
                currentIndex++;
             } else {
                Serial.print("Received checksum ");
                Serial.print(receivedByte, HEX);
                Serial.print(" at index ");
                Serial.println(currentIndex);
                //Validate the checksum
                if (calculateChecksum(tempRGB) == receivedByte) {
                    Serial.println("Checksum was valid");
                    /* It's valid. Set the new RGB values and restart the state
                     * machine.
                     */
                    setRGB(tempRGB[0], tempRGB[1], tempRGB[2]);
                    currentIndex = 0;
                } else {
                    Serial.println("Checksum was INVALID");
                    //The checksum is invalid. Restart the state machine.
                    currentIndex = 0;
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
    /*
     * TODO: make this actually set the RGB values
     */
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
}