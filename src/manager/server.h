/**
 * SIGMusic Lights 2015
 * Websocket server class
 */

#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>


typedef websocketpp::server<websocketpp::config::asio> ws_server;


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
    void send(websocketpp::connection_hdl client, const std::string message);

private:
    static void onMessage(websocketpp::connection_hdl client, ws_server::message_ptr msg);
    static bool shouldConnect(websocketpp::connection_hdl client);
    static ws_server * ws;
};