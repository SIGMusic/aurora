/**
 * SIGMusic Lights 2015
 * Websocket server class
 */

#pragma once

#include <semaphore.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <semaphore.h>
#include "manager.h"

typedef websocketpp::server<websocketpp::config::asio> ws_server;

class Server {
public:
    /**
     * Initializes a websocket server.
     */
    Server();

    /**
     * Runs the webserver I/O loop. Does not return.
     * 
     * @param colors The array of color values
     * @param colors_sem The semaphore for accessing the colors array
     * @param connected The array of connected lights
     * @param connected_sem The semaphore for accessing the connected array
     */
    void run(color_t* colors, sem_t* colors_sem,
        uint32_t* connected, sem_t* connected_sem);

private:

    static bool shouldConnect(websocketpp::connection_hdl client);
    static void onMessage(websocketpp::connection_hdl client, ws_server::message_ptr msg);
    static void processMessage(websocketpp::connection_hdl client, const std::string message);
    static bool isLightConnected(uint8_t id);
    
    static ws_server* ws;

    static color_t* colors;
    static sem_t* colors_sem;
    static uint32_t* connected;
    static sem_t* connected_sem;
};
