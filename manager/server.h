/**
 * SIGMusic Lights 2015
 * Websocket server class
 */

#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


typedef websocketpp::server<websocketpp::config::asio> server_t;


class Server {
public:
    /**
     * Initializes a websocket server.
     */
    Server();

    /**
     * Runs the webserver I/O loop. Does not return.
     */
    void run();

    /**
     * Sends a message to the connected client.
     *
     * @param message The message to send
     */
    void send(std::string message);

private:
    void onMessage(websocketpp::connection_hdl client, server_t::message_ptr msg);
    bool shouldConnect(websocketpp::connection_hdl client);
    server_t ws;
};
