/**
 * SIGMusic Lights 2016
 * Radio class
 */

#pragma once

#include <cstdint>
#include <RF24/RF24.h>
#include <semaphore.h>
#include "common.h"

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

    static void transmitFrame(int signal);
    static unsigned long millis();

    static RF24 radio;
    static struct shared* s;
};
