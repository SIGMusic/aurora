/**
 * SIGMusic Lights 2015
 * Radio class
 */

#include <iostream>
#include <arpa/inet.h>
#include "../network.h"
#include "radio.h"


// Radio pins
#define CE_PIN                  RPI_BPLUS_GPIO_J8_22 // Radio chip enable pin
#define CSN_PIN                 RPI_BPLUS_GPIO_J8_24 // Radio chip select pin


Radio::Radio() :
        _radio(CE_PIN, CSN_PIN),
        endpointID(BASE_STATION_ID) {

    _radio.begin();
    _radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    _radio.setChannel(CHANNEL);
    _radio.setPALevel(RF24_PA_MAX); // Range is important, not power consumption
    _radio.setRetries(0, NUM_RETRIES);
    _radio.setCRCLength(RF24_CRC_16);
    _radio.setPayloadSize(sizeof(packet_t));

    _radio.openReadingPipe(1, RF_ADDRESS(endpointID));
    _radio.startListening();

#ifdef DEBUG
    _radio.printDetails();
#endif
}

bool Radio::send(uint8_t endpoint, const Message & msg) {
    packet_t packet = {
        HEADER,
        msg.command,
        msg.data
    };

    _radio.stopListening();
    _radio.openWritingPipe(RF_ADDRESS(endpoint));
    bool success = _radio.write(&packet, sizeof(packet));
    _radio.startListening();

    return success;
}

bool Radio::receive(Message & msg, unsigned int timeout) {
    unsigned int started_waiting_at = millis();
    bool wasTimeout = false;

    while (!_radio.available() && !wasTimeout) {
        if ((millis() - started_waiting_at) > timeout) {
            wasTimeout = true;
        }
    }

    if (wasTimeout) {
        return false;
    }

    // Get the response
    packet_t response;
    _radio.read(&response, sizeof(packet_t));

    // Make sure the response is valid
    if (response.header != HEADER) {
        return false;
    }

    // Copy the response into the client's buffer
    msg.command = response.command;
    std::copy(std::begin(response.data), std::end(response.data), std::begin(msg.data));

    return true;
}
