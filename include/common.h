/**
 * SIGMusic Lights 2016
 * Raspberry Pi common definitions
 */

#pragma once
#include <semaphore.h>
#include <cstdint>

// Number of possible endpoint IDs
#define NUM_IDS         256

// Represents a 24-bit RGB color
typedef struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

// Stores data that needs to be shared between the radio and the server
struct shared {
    color_t colors[NUM_IDS];
    sem_t colors_sem;
};

/**
 * Returns true if the given light is currently connected.
 * 
 * @param connected The shared array of connected lights
 * @param id The endpoint ID to check
 * 
 * @return true if the light is connected, false otherwise
 */
extern bool isLightConnected(uint32_t* connected, uint8_t id);
