/**
 * SIGMusic Lights 2015
 * Common utilities
 */

#include <stdint.h>
#include "common.h"

bool isLightConnected(uint32_t* connected, uint8_t id) {

    int index = id / sizeof(uint32_t);
    int bit = id % sizeof(uint32_t);

    uint32_t flags = connected[index];
    uint32_t mask = 1 << bit;

    return (flags & mask);
}
