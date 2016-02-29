/**
 * SIGMusic Lights 2015
 * Common definitions
 */

#pragma once
#include <semaphore.h>

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
