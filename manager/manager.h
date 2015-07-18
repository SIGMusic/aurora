/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#pragma once

#include <websocketpp/server.hpp>

extern void processMessage(websocketpp::connection_hdl client, const std::string message);
