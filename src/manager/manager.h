/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#pragma once

#include <websocketpp/server.hpp>

#define MAX_FPS     30

typedef struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

extern void processMessage(websocketpp::connection_hdl client, const std::string message);
