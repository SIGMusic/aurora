/**
 * SIGMusic Lights 2015
 * Header file for network configuration
 */

#pragma once

#include <stdint.h>

// Generates a 40-bit nRF address
#define RF_ADDRESS(endpoint)    (0x5349474D00LL | ((endpoint) & 0xFF))

// The endpoint ID of the base station
#define BASE_STATION_ID         0x00

 // The radio channel to listen on: 2.4005 GHz + (Channel# * 1 MHz)
 // 0-83: legal, also used for WiFi and Bluetooth
 // 84-100: illegal
 // 101-125: potentially legal, lower power only
#define CHANNEL                 80

// A list of command bytes specified in the protocol
enum commands {
    CMD_SET_RGB             = 0x10,
    CMD_PING                = 0x80,
    CMD_PING_RESPONSE       = 0x81,
    CMD_GET_TEMP            = 0x90,
    CMD_TEMP_RESPONSE       = 0x91,
    CMD_GET_UPTIME          = 0x92,
    CMD_UPTIME_RESPONSE     = 0x93,
    CMD_GET_VERSION         = 0x94,
    CMD_VERSION_RESPONSE    = 0x95,
};

// The structure of a packet specified in the protocol
typedef struct packet {
    uint8_t command;
    uint8_t data[3];
} packet_t;
