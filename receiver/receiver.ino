/**
 * SIGMusic Lights 2015
 * Arduino receiver firmware
 */

#include "Arduino.h"
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>


//RGB pins
#define RED_PIN             9
#define GREEN_PIN           10
#define BLUE_PIN            11

//Radio pins
#define CE_PIN              2 //Radio chip enable pin
#define CSN_PIN             3 //Radio chip select pin

//Wireless protocol
#define NODE_RF_ADDRESS         ((uint64_t)'SIGMUSIC')
#define HEADER                  7446
#define BASE_STATION_ID         0x00

enum COMMANDS {
    CMD_SET_RGB       = 0x10,
    CMD_BROADCAST_RGB = 0x11,
    CMD_PING          = 0x20,
    CMD_PING_RESPONSE = 0x21,
};

//Serial protocol
#define MAX_SERIAL_LINE_LEN 20


#define VERSION                 0 //The firmware version number
#define ENDPOINT_ID_LOCATION    0x00 //The address in EEPROM to store the ID


typedef struct packet_tag {
    uint16_t header;
    uint8_t command;
    uint8_t endpoint;
    uint8_t rgb[3];
} packet_t;


void networkRead(void);
void processPacket(packet_t packet);
void serialRead(void);
void processCommand(char message[]);
void setRGB(uint8_t red, uint8_t green, uint8_t blue);
void setEndpointID(uint8_t id);
void welcomeMessage(void);
void helpMessage(void);


RF24 radio(CE_PIN, CSN_PIN);

uint8_t endpointID = EEPROM.read(ENDPOINT_ID_LOCATION);

uint8_t lastPingEndpoint;
unsigned long lastPingTime;

/**
 * Initialize SPI, radio, and serial.
 */
void setup() {
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(16); // Set SPI clock to 16 MHz / 16 = 1 MHz

    // nRF24L01+ radio initialization
    radio.begin();
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(80); // Channel center frequency = 2.4005 GHz + (Channel# * 1 MHz)
    radio.setPALevel(RF24_PA_MAX);
    radio.setRetries(0, 0);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_8);
    radio.setPayloadSize(sizeof(packet_t));
    radio.openReadingPipe(0, NODE_RF_ADDRESS);
    radio.startListening();

    Serial.begin(9600);
    welcomeMessage();
}

/**
 * Continuously read in data from the network and from serial.
 */
void loop() {
    networkRead();
    serialRead();
}

/**
 * Reads network data into a struct and sends it to be processed.
 */
void networkRead(void) {
    if (radio.available()) {
        packet_t packet;
        radio.read(&packet, sizeof(packet_t));
        processPacket(packet);
    }
}

/**
 * Takes action on a packet received over the network.
 * @param packet The packet received
 */
void processPacket(packet_t packet) {
    if (packet.header != HEADER) {
        //Invalid packet
        return;
    }

    if ((packet.command == CMD_SET_RGB && packet.endpoint == endpointID) ||
        packet.command == CMD_BROADCAST_RGB) {

        setRGB(packet.rgb[0], packet.rgb[1], packet.rgb[2]);
    } else if (packet.command == CMD_PING && packet.endpoint == endpointID) {
        //Received a ping request. Respond with the endpoint ID and version.
        packet_t response = {
            HEADER,
            CMD_PING_RESPONSE,
            BASE_STATION_ID,
            {endpointID, VERSION, 0}
        };

        radio.stopListening();
        radio.openWritingPipe(NODE_RF_ADDRESS);
        radio.write(&response, sizeof(response));
        radio.openReadingPipe(0, NODE_RF_ADDRESS);
    } else {
        //Ignore packet
    }
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
            buffer[index] = '\0'; //Terminate the message
            index = 0; //Reset to the beginning of the buffer
            processCommand(buffer);
        } else {
            index++;
        }
    }
}

/**
 * Takes action on a command received over serial.
 * @param message The message received
 */
void processCommand(char message[]) {
    uint8_t id;
    if (!strcmp(message, "help")) {
        helpMessage();
    } else if (!strcmp(message, "version")) {
        Serial.println(VERSION);
    } else if (!strcmp(message, "getid")) {
        Serial.println(endpointID, HEX);
    } else if (sscanf(message, "setid %2x", &id) == 1) {
        setEndpointID(id);
        Serial.println("OK");
    } else {
        Serial.println("Error: unrecognized command");
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
    EEPROM.write(ENDPOINT_ID_LOCATION, id);
    endpointID = id;
}

/**
 * Prints the welcome message for serial debugging.
 */
void welcomeMessage(void) {
    Serial.println(F("-------------------------------------"));
    Serial.println(F("SIGMusic@UIUC Lights Serial Interface"));
    Serial.print(F("Version "));
    Serial.println(VERSION);
    Serial.print(F("This light is endpoint 0x"));
    Serial.println(endpointID, HEX);
    Serial.println(F("End all commands with a carriage return."));
    Serial.println(F("Type 'help' for a list of commands."));
    Serial.println(F("-------------------------------------"));
}

/**
 * Prints the help message for serial debugging.
 */
void helpMessage(void) {
    Serial.println(F("Available commands:"));
    Serial.println(F("  help - displays this help message"));
    Serial.println(F("  version - displays the firmware version number"));
    Serial.println(F("  setid [id] - sets the endpoint ID"));
    Serial.println(F("      [id] - the new ID (in hexadecimal)"));
    Serial.println(F("  getid - displays the endpoint ID"));
}
