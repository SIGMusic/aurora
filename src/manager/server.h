/**
 * SIGMusic Lights 2016
 * Websocket server class
 */

#pragma once

#include <semaphore.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <semaphore.h>
#include "common.h"

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
     * @param s The struct of shared memory
     */
    void run(struct shared* s);

private:

    static bool shouldConnect(websocketpp::connection_hdl client);
    static void onMessage(websocketpp::connection_hdl client, ws_server::message_ptr msg);
    static void processMessage(websocketpp::connection_hdl client, const std::string message);
    
    static ws_server* ws;
    static struct shared* s;
};
