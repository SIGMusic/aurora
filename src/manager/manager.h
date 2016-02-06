/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#pragma once

// The FPS limit
#define MAX_FPS         30

// Number of possible endpoint IDs
#define NUM_IDS                 256

typedef struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;
