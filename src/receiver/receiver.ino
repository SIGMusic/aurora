/**
 * SIGMusic Lights 2015
 * Arduino receiver firmware
 */

#include "Arduino.h"
#include <SPI.h>
#include <EEPROM.h>
#include <RF24.h>
#include "network.h"

// RGB pins
// Note: pins 5 and 6 use the fast, inaccurate PWM timer. If this becomes
// an issue, look into software PWM implementations.
#define RED_PIN                 3
#define GREEN_PIN               5
#define BLUE_PIN                6

// Radio pins
#define CE_PIN                  7 // Radio chip enable pin
#define CSN_PIN                 8 // Radio chip select pin
#define INT_PIN                 2 // Radio interrupt pin

// Serial protocol
#define MAX_SERIAL_LINE_LEN     20 // Null terminator is already accounted for

// Firmware information
#define ENDPOINT_ID_LOCATION    0x000 // The address in EEPROM to store the ID


void initRadio(void);
uint8_t detectClearestChannel(void);
// void networkRead(void);
void radioInterrupt(void);
void serialRead(void);
void processSerialCommand(char message[]);
void setRGB(uint8_t red, uint8_t green, uint8_t blue);
void setEndpointID(uint8_t id);
void printWelcomeMessage(void);
void printHelpMessage(void);


RF24 radio(CE_PIN, CSN_PIN);

uint8_t endpointID = EEPROM.read(ENDPOINT_ID_LOCATION);

/**
 * Initialize radio and serial.
 */
void setup() {

    // Initialize serial console
    Serial.begin(115200);
    printWelcomeMessage();

    // Start the radio
    initRadio();
}

/**
 * Continuously read in data from the network and from serial.
 */
void loop() {
    serialRead();
}

/**
 * Initializes the radio.
 */
void initRadio(void) {
    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setPALevel(RF24_PA_LOW); // Higher power doesn't work on cheap eBay radios
    radio.setRetries(0, 0);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.setChannel(49);
    attachInterrupt(digitalPinToInterrupt(INT_PIN), radioInterrupt, LOW);

    radio.openReadingPipe(1, RF_ADDRESS(endpointID));
    radio.startListening();
}

/**
 * Reads network data into a struct and sends it to be processed.
 */
void radioInterrupt(void) {

    packet_t packet;
    radio.read(&packet, sizeof(packet));

    Serial.print(packet.data[0], HEX);
    Serial.print(" ");
    Serial.print(packet.data[1], HEX);
    Serial.print(" ");
    Serial.println(packet.data[2], HEX);

    setRGB(packet.data[0], packet.data[1], packet.data[2]);
}

/**
 * Reads serial data into a buffer. When a carriage return is encountered,
 * the message is sent to be processed.
 */
void serialRead(void) {

    static char buffer[MAX_SERIAL_LINE_LEN + 1];
    static int index = 0;

    if (Serial.available()) {

        buffer[index] = Serial.read();

        if (buffer[index] == '\r' ||  index == MAX_SERIAL_LINE_LEN) {
            buffer[index] = '\0'; // Terminate the message
            index = 0; // Reset to the beginning of the buffer
            processSerialCommand(buffer);
        } else {
            index++;
        }
    }
}

/**
 * Takes action on a command received over serial.
 * @param message The message received
 */
void processSerialCommand(char message[]) {

    uint8_t id, r, g, b;
    if (!strcmp(message, "help")) {
        printHelpMessage();
    } else if (!strcmp(message, "getid")) {
        Serial.println(endpointID);
        Serial.println(F("OK"));
    } else if (sscanf(message, "setid %hhu", &id) == 1) {
        setEndpointID(id);
        Serial.println(F("OK"));
    } else if (sscanf(message, "setrgb %hhu %hhu %hhu", &r, &g, &b) == 3) {
        setRGB(r, g, b);
        Serial.println(F("OK"));
    } else {
        Serial.println(F("Error: unrecognized command or invalid arguments"));
    }
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
}

/**
 * Sets the value of the endpoint ID in EEPROM.
 * @param id The new endpoint ID
 */
void setEndpointID(uint8_t id) {

    // Make sure it's not the base station ID
    if (id == BASE_STATION_ID) {
        return;
    }

    EEPROM.write(ENDPOINT_ID_LOCATION, id);
    endpointID = id;
    radio.openReadingPipe(1, RF_ADDRESS(endpointID));
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {
    Serial.println(F("-----------------------------------------"));
    Serial.println(F("ACM@UIUC SIGMusic Lights Serial Interface"));
    Serial.print(F("This light is endpoint "));
    Serial.println(endpointID);
    Serial.println(F("End all commands with a carriage return."));
    Serial.println(F("Type 'help' for a list of commands."));
    Serial.println(F("-----------------------------------------"));
}

/**
 * Prints the help message for the serial console.
 */
void printHelpMessage(void) {
    Serial.println(F("Available commands:"));
    Serial.println(F("  help - displays this help message"));
    Serial.println(F("  setrgb [red] [green] [blue] - sets the color"));
    Serial.println(F("      [red], [green], [blue] - the value of each channel (0 to 255)"));
    Serial.println(F("  setid [id] - sets the endpoint ID"));
    Serial.println(F("      [id] - the new ID (1 to 255)"));
    Serial.println(F("  getid - displays the endpoint ID in hex"));
}
