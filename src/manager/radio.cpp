/**
 * SIGMusic Lights 2015
 * Radio class
 */

#include <iostream>
#include <arpa/inet.h>
#include "radio.h"
#include "../network.h"


using std::cout;
using std::endl;


// Radio pins
#define CE_PIN                  RPI_V2_GPIO_P1_15 // Radio chip enable pin
#define CSN_PIN                 RPI_V2_GPIO_P1_24 // Radio chip select pin

// Cap on the number of light updates per second
#define MAX_FPS         10


RF24 Radio::radio(CE_PIN, CSN_PIN);

struct shared* Radio::s;


Radio::Radio() {

    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setPALevel(RF24_PA_LOW); // Higher power doesn't work on cheap eBay radios
    radio.setRetries(0, 0);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.setChannel(49);

    radio.openReadingPipe(1, RF_ADDRESS(BASE_STATION_ID));

#ifdef DEBUG
    radio.printDetails();
#endif
}

void Radio::run(struct shared* s) {
    
    Radio::s = s;

    clock_t last_update = clock();
    while (1) {

        // Only transmit once per frame
        clock_t now = clock();
        if ((((float)now)/CLOCKS_PER_SEC -
            ((float)last_update)/CLOCKS_PER_SEC) > 1.0/MAX_FPS) {

            transmitFrame();
            last_update = now;
        }
    }
}

void Radio::transmitFrame() {

    static int lastChannelIndex = -1;

    sem_wait(&s->colors_sem);

    for (int i = 1; i <= 1; i++) {

        // Make sure we're on the right channel
        // uint32_t now = millis();
        uint32_t now = millis();
        int channelIndex = (now / DWELL_TIME) % NUM_CHANNELS;

        // if (channelIndex != lastChannelIndex) {
        //     radio.setChannel(channelIndex);
        //     lastChannelIndex = channelIndex;
        //     cout << "Channel " << channelIndex << endl;
        // }

        cout << (now) << endl;
        
        packet_t msg = {
            {(uint8_t)channelIndex, 0, 0},
            // {s->colors[i].r, s->colors[i].g, s->colors[i].b},
            (uint8_t)(now % DWELL_TIME)
        };

        radio.openWritingPipe(RF_ADDRESS(i));
        radio.write(&msg, sizeof(msg));
    }

    sem_post(&s->colors_sem);
}

uint32_t Radio::millis() {

    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
    return (tv.tv_sec * 1000) + (tv.tv_nsec / 1000000);
}

uint64_t Radio::micros() {

    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
    return (tv.tv_sec * 1000000) + (tv.tv_nsec / 1000);
}
