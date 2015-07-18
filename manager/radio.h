/**
 * SIGMusic Lights 2015
 * Radio class
 */

#pragma once

#include <cstdint>
#include <RF24/RF24.h>
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

        uint8_t command;
        uint8_t data[3];
    };

    /**
     * Initializes the radio.
     */
    Radio();

    /**
     * Sends a message to an endpoint.
     *
     * @param endpoint The endpoint to send the message to
     * @param msg The message to be sent
     *
     * @return False if the message could not be sent, else true
     */
    bool send(uint8_t endpoint, const Message & msg);

    /**
     * Blocks until a message is received or the timeout has passed.
     *
     * @param msg The place for the received message to be stored
     * @param timeout The time in milliseconds to wait for a response
     *
     * @return False if the timeout occurred, else true
     */
    bool receive(Message & msg, unsigned int timeout);

private:
    RF24 _radio;
    uint8_t endpointID;
};
