/**
 * SIGMusic Lights 2015
 * Common definitions
 */

#pragma once
#include <semaphore.h>

// The software version number
#define SW_VERSION      0

// Cap on the number of light updates per second
#define MAX_FPS         30

// Number of possible endpoint IDs
#define NUM_IDS         256

// The length of the connected array
#define CONNECTED_LEN   ((NUM_IDS + sizeof(uint32_t) - 1)/sizeof(uint32_t))

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

    uint32_t connected[CONNECTED_LEN];
    sem_t connected_sem;
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
