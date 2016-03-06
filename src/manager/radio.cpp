/**
 * SIGMusic Lights 2015
 * Radio class
 */

#include <iostream>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
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

    timer_t timer;
    struct sigaction sa;
    struct itimerspec its;

    // Create the timer
    if (timer_create(CLOCK_MONOTONIC, NULL, &timer)) {
        perror("timer_create");
        exit(1);
    }

    // Set up the SIGALRM
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = Radio::transmitFrame;
    sigaction(SIGALRM, &sa, NULL);

    // Calculate the timer interval
    double period = 1.0/MAX_FPS;
    its.it_interval.tv_sec = (time_t) period;
    its.it_interval.tv_nsec = (long) (period * 1000000000) % 1000000000;
    its.it_value.tv_sec = its.it_interval.tv_sec;
    its.it_value.tv_nsec = its.it_interval.tv_nsec;

    // Start the timer
    if (timer_settime(timer, 0, &its, NULL)) {
        perror("timer_settime");
        exit(1);
    }

    while (1) {
        sleep(1000);
    }
}

void Radio::transmitFrame(int signal) {

    static int lastChannelIndex = 0;

    // Locking the semaphore in a signal handler should be okay as long as
    // the webserver only locks it for short bursts.
    sem_wait(&s->colors_sem);

    for (int i = 1; i <= 1; i++) {

        // Make sure we're on the right channel
        uint32_t now = millis();
        int channelIndex = (now / DWELL_TIME) % NUM_CHANNELS;

        if (channelIndex != lastChannelIndex) {
            // radio.setChannel(channelIndex);
            lastChannelIndex = channelIndex;
            cout << "Channel " << channelIndex << endl;
        }

        // cout << (now) << endl;
        
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
