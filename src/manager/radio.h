/**
 * SIGMusic Lights 2015
 * Radio class
 */

#pragma once

#include <cstdint>
#include <RF24/RF24.h>
#include <semaphore.h>
#include "manager.h"
#include "../network.h"

class Radio {
public:
    /**
     * Stores a message consisting of a command and 3 data bytes.
     */
    struct Message {
        Message() :
                command(0),
                data{}
            { }

        Message(uint8_t cmd) :
                command(cmd),
                data{}
            { }

        Message(uint8_t cmd, uint8_t d[3]) :
                command(cmd),
                data{d[0], d[1], d[2]}
            { }

        Message(uint8_t cmd, color_t c) :
                command(cmd),
                data{c.r, c.g, c.b}
            { }

        uint8_t command;
        uint8_t data[3];
    };

    /**
     * Initializes the radio.
     */
    Radio();

    /**
     * Runs the radio control loop. Does not return.
     * 
     * @param colors The array of color values
     * @param colors_sem The semaphore for accessing the colors array
     * @param connected The array of connected lights
     * @param connected_sem The semaphore for accessing the connected array
     */
    void run(color_t* colors, sem_t* colors_sem,
        uint32_t* connected, sem_t* connected_sem);

private:

    static void checkPipe();
    static bool send(uint8_t endpoint, const Message & msg);
    static bool receive(Message & msg, unsigned int timeout);
    static void pingAllLights();
    static void setLightConnected(uint8_t id, bool isConnected);

    static bool getTemperature(uint8_t endpoint, int16_t * temp);
    static bool getUptime(uint8_t endpoint, uint16_t * uptime);
    static bool setRGB(uint8_t endpoint, uint8_t red, uint8_t green, uint8_t blue);

    static RF24 radio;

    static color_t* colors;
    static sem_t* colors_sem;
    static uint32_t* connected;
    static sem_t* connected_sem;
};
