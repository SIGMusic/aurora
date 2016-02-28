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

// The structure of a packet specified in the protocol
typedef struct packet {
    uint8_t data[3];
} packet_t;
