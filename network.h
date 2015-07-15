/**
 * SIGMusic Lights 2015
 * Header file for network configuration
 */

#pragma once

#include <stdint.h>

// Simple 1:1 single byte hash to minimize repeating bit patterns in address
#define HASH(a)                 ((a) ^ 73)

// Generates a 40-bit nRF address
#define RF_ADDRESS(endpoint)    (0x5349474D00LL | HASH(endpoint & 0xFF))

// The endpoint ID of the base station
#define BASE_STATION_ID         0x00

// Must prefix every message
#define HEADER                  ((uint16_t)htons(7446))

 // The radio channel to listen on: 2.4005 GHz + (Channel# * 1 MHz)
#define CHANNEL                 80

// The number of tries to wait for acknowledgement before giving up
#define NUM_RETRIES             5

// A list of command bytes specified in the protocol
enum COMMANDS {
    CMD_SET_RGB         = 0x10,
    CMD_PING            = 0x20,
    CMD_PING_RESPONSE   = 0x21,
    CMD_GET_TEMP        = 0x30,
    CMD_TEMP_RESPONSE   = 0x31,
    CMD_GET_UPTIME      = 0x32,
    CMD_UPTIME_RESPONSE = 0x33,
};

// The structure of a packet specified in the protocol
typedef struct packet_tag {
    uint16_t header;
    uint8_t command;
    uint8_t data[3];
} packet_t;
