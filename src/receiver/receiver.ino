/**
 * SIGMusic Lights 2016
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

// Firmware information
#define ENDPOINT_ID_LOCATION    0x000 // The address in EEPROM to store the ID


void initRadio(void);
uint8_t detectClearestChannel(void);
void radioInterrupt(void);

void serialRead(void);
void processSerialCommand(String message);

void setRGB(uint8_t red, uint8_t green, uint8_t blue);

void setEndpointID(uint8_t id);
uint8_t getEndpointID(void);

void printWelcomeMessage(void);
void printHelpMessage(void);


RF24 radio(CE_PIN, CSN_PIN);

/**
 * Initialize radio and serial.
 */
void setup() {

    Serial.begin(115200);
    Serial.setTimeout(-1);

    initRadio();

    setRGB(0, 0, 0);

    printWelcomeMessage();
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

    radio.setChannel(RF_CHANNEL);
    attachInterrupt(digitalPinToInterrupt(INT_PIN), radioInterrupt, LOW);

    radio.openReadingPipe(1, RF_ADDRESS(getEndpointID()));
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

    String message = Serial.readStringUntil('\r');

    if (!message.equals("")) {

        processSerialCommand(message);
        Serial.read(); // Clear out the carriage return
    }
}

/**
 * Takes action on a command received over serial.
 * @param message The message received
 */
void processSerialCommand(String message) {

    uint8_t id, r, g, b;

    if (message.equals("help")) {

        printHelpMessage();

    } else if (message.equals("getid")) {

        Serial.println(getEndpointID());
        Serial.println(F("OK"));

    } else if (sscanf(message.c_str(), "setid %hhu", &id) == 1) {

        setEndpointID(id);
        Serial.println(F("OK"));

    } else if (sscanf(message.c_str(), "setrgb %hhu %hhu %hhu", &r, &g, &b) == 3) {

        setRGB(r, g, b);
        Serial.println(F("OK"));

    } else {

        Serial.print(F("Error: unrecognized command or invalid arguments: "));
        Serial.println(message);
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
    radio.openReadingPipe(1, RF_ADDRESS(id));
}

/**
 * Reads the value of the endpoint ID in EEPROM.
 * 
 * @return the endpoint ID
 */
uint8_t getEndpointID(void) {

    return EEPROM.read(ENDPOINT_ID_LOCATION);
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {

    Serial.println();
    Serial.println(F("-----------------------------------------"));
    Serial.println(F("ACM@UIUC SIGMusic Lights Serial Interface"));
    Serial.print(F("This light is endpoint "));
    Serial.println(getEndpointID());
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
