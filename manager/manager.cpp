/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <arpa/inet.h>
#include <RF24/RF24.h>
#include "../network.h"


// Radio pins
#define CE_PIN                  RPI_BPLUS_GPIO_J8_22 // Radio chip enable pin
#define CSN_PIN                 RPI_BPLUS_GPIO_J8_24 // Radio chip select pin

// Wireless protocol
#define ENDPOINT_PIPE           1 // The pipe for messages for this endpoint
#define PING_TIMEOUT            10 // Time to wait for a ping response in milliseconds

// Software information
#define VERSION                 0 // The software version number


void pingAllLights(void);
void printWelcomeMessage(void);


RF24 radio(CE_PIN, CSN_PIN);

uint8_t endpointID = BASE_STATION_ID;
bool lights[255] = {0};


int main(int argc, char** argv){
    // Initialize the radio
    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setChannel(CHANNEL);
    radio.setPALevel(RF24_PA_MAX); // Range is important, not power consumption
    radio.setRetries(0, NUM_RETRIES);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.openReadingPipe(ENDPOINT_PIPE, RF_ADDRESS(endpointID));
    radio.startListening();

    printWelcomeMessage();

    radio.printDetails();

    puts("Scanning for lights...");
    pingAllLights();
    puts("Done scanning.");


    // Broadcast red to all lights as a test
    packet_t red = {
        HEADER,
        CMD_SET_RGB,
        {255, 0, 0}
    };
    radio.stopListening();
    radio.openWritingPipe(RF_ADDRESS(MULTICAST_ID));
    radio.write(&red, sizeof(packet_t));
    radio.startListening();

    /**
     * Main loop
     */
    // for (;;) {
    //     // TODO
    // }

    return 0;
}

/**
 * Pings every endpoint and records the ones that respond.
 */
void pingAllLights(void) {
    // Generate the ping packet
    packet_t ping = {
        HEADER,
        CMD_PING,
        {0, 0, 0}
    };

    for (int i = 0; i < 0xff; i++) {
        radio.stopListening();
        radio.openWritingPipe(RF_ADDRESS(i));
        bool success = radio.write(&ping, sizeof(packet_t));
        radio.startListening();

        // If the packet wasn't delivered after several attempts, move on
        if (!success) {
            continue;
        }
        
        // Wait here until we get a response or timeout
        unsigned int started_waiting_at = millis();
        bool timeout = false;
        while (!radio.available() && !timeout) {
            if ((millis() - started_waiting_at) > PING_TIMEOUT) {
                timeout = true;
            }
        }

        // Skip endpoints that timed out
        if (timeout) {
            continue;
        }

        // Get the response
        packet_t response;
        radio.read(&response, sizeof(packet_t));

        // Make sure the response checks out
        if (response.header != HEADER ||
            response.command != CMD_PING_RESPONSE ||
            response.data[0] != i) {
            continue;
        }

        printf("Found light 0x%02x\n", i);

        // Add the endpoint to the list
        lights[i] = true;
    }
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {
    puts("-------------------------------------");
    puts("SIGMusic@UIUC Lights Base Station");
    printf("Version %x\n", VERSION);
    puts("This base station is endpoint 0x00");
    puts("-------------------------------------");
}
