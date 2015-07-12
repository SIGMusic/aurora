/**
 * SIGMusic Lights 2015
 * Header file for network configuration
 */

#pragma once

#define HASH(a)                 ((a) ^ 73) // Simple 1:1 single byte hash to minimize repeating bit patterns in address
#define RF_ADDRESS(endpoint)    (0x5349474D00LL | HASH(endpoint & 0xFF)) // Generates a 40-bit nRF address
#define BASE_STATION_ID         0x00 // The endpoint ID of the base station
#define MULTICAST_ID            0xFF // The endpoint ID that all lights listen to

#define HEADER                  htons(7446) // Must prefix every message
#define CHANNEL                 80 // Channel center frequency = 2.4005 GHz + (Channel# * 1 MHz)
#define NUM_RETRIES             5

enum COMMANDS {
    CMD_SET_RGB       = 0x10,
    CMD_PING          = 0x20,
    CMD_PING_RESPONSE = 0x21,
};

typedef struct packet_tag {
    uint16_t header;
    uint8_t command;
    uint8_t data[3];
} packet_t;
