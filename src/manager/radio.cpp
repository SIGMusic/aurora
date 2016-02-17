/**
 * SIGMusic Lights 2015
 * Radio class
 */

#include <iostream>
#include <arpa/inet.h>
#include "radio.h"


using std::cout;
using std::endl;


// Radio pins
#define CE_PIN                  RPI_V2_GPIO_P1_15 // Radio chip enable pin
#define CSN_PIN                 RPI_V2_GPIO_P1_24 // Radio chip select pin

// Wireless protocol
#define PING_TIMEOUT            4 // Time to wait for a ping response in milliseconds


RF24 Radio::radio(CE_PIN, CSN_PIN);

struct shared* Radio::s;


Radio::Radio() {

    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setChannel(CHANNEL);
    radio.setPALevel(RF24_PA_LOW); // Higher power doesn't work on cheap eBay radios
    radio.setRetries(0, 0);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.openReadingPipe(1, RF_ADDRESS(BASE_STATION_ID));

#ifdef DEBUG
    radio.printDetails();
#endif
}

void Radio::run(struct shared* s) {
    
    Radio::s = s;

    pingAllLights();

    clock_t last_update = clock();
    while(1) {
        
        // Only transmit once per frame
        clock_t now = clock();
        if ((((float)now)/CLOCKS_PER_SEC -
            ((float)last_update)/CLOCKS_PER_SEC) > 1.0/MAX_FPS) {

            transmitFrame();
            last_update = now;
        }
    }
}

void Radio::send(uint8_t endpoint, packet_t & msg) {

    radio.openWritingPipe(RF_ADDRESS(endpoint));
    radio.write(&msg, sizeof(msg));
}

bool Radio::receive(packet_t & response, unsigned int timeout) {

    unsigned int started_waiting_at = millis();
    bool wasTimeout = false;

    radio.startListening();

    while (!radio.available() && !wasTimeout) {
        if ((millis() - started_waiting_at) > timeout) {
            wasTimeout = true;
        }
    }

    if (wasTimeout) {
        radio.stopListening();
        return false;
    }

    // Get the response
    radio.read(&response, sizeof(packet_t));

    radio.stopListening();

    return true;
}

/**
 * Pings every endpoint and records the ones that respond.
 */
void Radio::pingAllLights() {

    cout << "Scanning for lights..." << endl;

    sem_wait(&s->connected_sem);

    for (int i = 1; i < NUM_IDS; i++) {

        packet_t ping = {
            CMD_PING,
            {0, 0, 0}
        };

        send(i, ping);

        packet_t response;
        
        // Wait here until we get a response or timeout
        if (!receive(response, PING_TIMEOUT)) {
            setLightConnected(i, false);
            continue;
        }

        // Make sure the response checks out
        if (response.command != CMD_PING_RESPONSE) {
            setLightConnected(i, false);
            continue;
        }

        printf("Found light %d\n", i);

        // Add the endpoint to the list
        setLightConnected(i, true);
    }

    sem_post(&s->connected_sem);

    cout << "Done scanning." << endl;
     
}

void Radio::transmitFrame() {

    sem_wait(&s->colors_sem);
    sem_wait(&s->connected_sem);

    for (int i = 1; i < NUM_IDS; i++) {
        
        // Only transmit to connected lights
        if (isLightConnected(s->connected, i)) {
            packet_t msg = {
                CMD_SET_RGB,
                {s->colors[i].r, s->colors[i].g, s->colors[i].b}
            };
            send(i, msg);
        }
    }

    sem_post(&s->connected_sem);
    sem_post(&s->colors_sem);
}

/**
 * Changes the connected status of the light.
 *
 * @param id The light to change
 * @param isConnected True if the light is now connected, false otherwise
 */
void Radio::setLightConnected(uint8_t id, bool isConnected) {

    int index = id / sizeof(uint32_t);
    int bit = id % sizeof(uint32_t);

    uint32_t mask = 1 << bit;

    if (isConnected) {
        s->connected[index] |= mask;
    } else {
        s->connected[index] &= ~mask;
    }
}
