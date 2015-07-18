/**
 * SIGMusic Lights 2015
 * Websocket server class
 */

#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;


class Server {
public:
    /**
     * Initializes a websocket server.
     */
    Server();

    /**
     * Sends a message to the connected client.
     *
     * @param message The message to send
     */
    void send(string message);

private:
    void onMessage(connection_hdl hdl, server::message_ptr msg);
    bool shouldConnect(connection_hdl hdl);
    server ws;
};
