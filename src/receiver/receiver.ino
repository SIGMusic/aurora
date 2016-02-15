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

// Serial protocol
#define MAX_SERIAL_LINE_LEN     20 // Null terminator is already accounted for

// Firmware information
#define VERSION                 0 // The firmware version number
#define ENDPOINT_ID_LOCATION    0x000 // The address in EEPROM to store the ID


void initRadio(void);
void networkRead(void);
void processNetworkPacket(packet_t packet);
void serialRead(void);
void processSerialCommand(char message[]);
void setRGB(uint8_t red, uint8_t green, uint8_t blue);
void setEndpointID(uint8_t id);
long getTemperature(void);
void printWelcomeMessage(void);
void printHelpMessage(void);


RF24 radio(CE_PIN, CSN_PIN);

uint8_t endpointID = EEPROM.read(ENDPOINT_ID_LOCATION);

/**
 * Initialize radio and serial.
 */
void setup() {
    // Start the radio
    initRadio();

    // Initialize serial console
    Serial.begin(9600);
    printWelcomeMessage();
}

/**
 * Continuously read in data from the network and from serial.
 */
void loop() {
    networkRead();
    serialRead();
}

/**
 * Initializes the radio.
 */
void initRadio(void) {
    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setChannel(CHANNEL);
    radio.setPALevel(RF24_PA_MAX); // Range is important, not power consumption
    radio.setRetries(0, 0);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.openWritingPipe(RF_ADDRESS(BASE_STATION_ID));
    radio.openReadingPipe(1, RF_ADDRESS(endpointID));
    radio.startListening();
}

/**
 * Reads network data into a struct and sends it to be processed.
 */
void networkRead(void) {
    if (radio.available()) {
        packet_t packet;
        radio.read(&packet, sizeof(packet_t));
        processNetworkPacket(packet);
    }
}

/**
 * Takes action on a packet received over the network.
 * @param packet The packet received
 */
void processNetworkPacket(packet_t packet) {

    switch (packet.command) {

        case CMD_SET_RGB: {
            // Update the color
            setRGB(packet.data[0], packet.data[1], packet.data[2]);
            break;
        }

        case CMD_PING: {
            // Respond with an echo of the received ping
            packet_t response = {
                CMD_PING_RESPONSE,
                {packet.data[0], packet.data[1], packet.data[2]}
            };

            radio.stopListening();
            radio.write(&response, sizeof(response));
            radio.startListening();

            break;
        }

        case CMD_GET_TEMP: {
            // Respond with the temperature
            long temp = getTemperature();
            packet_t response = {
                CMD_TEMP_RESPONSE,
                {(temp & 0xff00) >> 8, temp & 0xff, 0}
            };

            radio.stopListening();
            radio.write(&response, sizeof(response));
            radio.startListening();

            break;
        }

        case CMD_GET_UPTIME: {
            // Respond with the uptime
            unsigned long uptime = millis();
            packet_t response = {
                CMD_UPTIME_RESPONSE,
                {(uptime & 0xff00) >> 8, uptime & 0xff, 0}
            };

            radio.stopListening();
            radio.write(&response, sizeof(response));
            radio.startListening();

            break;
        }

        default:
            // Ignore the packet
            break;
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
 * Gets the (very approximate) core temperature in millidegrees Celsius.
 * @return The core temperature
 */
long getTemperature(void) {
    unsigned int wADC;

    // The internal temperature has to be used
    // with the internal reference of 1.1V.
    // Channel 8 cannot be selected with
    // the analogRead function yet.

    // Set the internal reference and mux.
    ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
    ADCSRA |= _BV(ADEN);  // enable the ADC

    delay(20);            // wait for voltages to become stable.

    ADCSRA |= _BV(ADSC);  // Start the ADC

    // Detect end-of-conversion
    while (bit_is_set(ADCSRA,ADSC));

    // Reading register "ADCW" takes care of how to read ADCL and ADCH.
    wADC = ADCW;

    // Approximate offset. Should be calibrated further.
    return (long)((wADC - 289)*1000/1.06);
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {
    Serial.println(F("-----------------------------------------"));
    Serial.println(F("ACM@UIUC SIGMusic Lights Serial Interface"));
    Serial.print(F("Version "));
    Serial.println(VERSION);
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
