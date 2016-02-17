/**
 * SIGMusic Lights 2015
 * Radio class
 */

#pragma once

#include <cstdint>
#include <RF24/RF24.h>
#include <semaphore.h>
#include "common.h"
#include "../network.h"

class Radio {
public:

    /**
     * Initializes the radio.
     */
    Radio();

    /**
     * Runs the radio control loop. Does not return.
     * 
     * @param s The struct of shared memory
     */
    void run(struct shared* s);

private:

    static void send(uint8_t endpoint, packet_t & msg);
    static bool receive(packet_t & msg, unsigned int timeout);
    static void pingAllLights();
    static void transmitFrame();
    static void setLightConnected(uint8_t id, bool isConnected);

    static RF24 radio;
    static struct shared* s;
};
