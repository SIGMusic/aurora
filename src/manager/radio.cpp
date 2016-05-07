/**
 * SIGMusic Lights 2016
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
#define MAX_FPS         120


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

    radio.setChannel(RF_CHANNEL);

    radio.openReadingPipe(1, RF_ADDRESS(BASE_STATION_ID));

#ifdef DEBUG
    radio.printDetails();
#endif
}

void ignoreSignal(int signal) {
    return;
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
    sa.sa_handler = ignoreSignal;
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

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);

    while (1) {
        int sig;
        sigwait(&set, &sig);
        puts("Alarm");
        transmitFrame();
    }
}

void Radio::transmitFrame() {

    sem_wait(&s->colors_sem);

    for (int i = 1; i <= 8; i++) {
        
        packet_t msg = {
            {s->colors[i].r, s->colors[i].g, s->colors[i].b}
        };

        radio.openWritingPipe(RF_ADDRESS(i));
        radio.write(&msg, sizeof(msg));
    }

    sem_post(&s->colors_sem);
}
