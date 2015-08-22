/**
 * SIGMusic Lights 2015
 * Websocket server class
 */

#include <string>
#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include "server.h"
#include "manager.h"


using websocketpp::connection_hdl;
using namespace websocketpp::frame;
using std::cout;
using std::endl;
using std::vector;
using std::string;


#define WS_PROTOCOL_NAME        "nlcp" // Networked Lights Control Protocol
#define WS_PORT                 7446


ws_server * Server::ws;


Server::Server() {
    ws = new ws_server();
    ws->set_message_handler(&onMessage);
    ws->set_validate_handler(&shouldConnect);

    ws->init_asio();
    ws->listen(WS_PORT);
    ws->start_accept();
}

void Server::run(void) {
    ws->run();
}

void Server::send(connection_hdl client, const string message) {
    ws->send(client, message, opcode::text);
}

void Server::onMessage(connection_hdl client, ws_server::message_ptr msg) {
    processMessage(client, msg->get_payload());
}

bool Server::shouldConnect(connection_hdl client) {
    // Get the connection so we can get info about it
    ws_server::connection_ptr connection = ws->get_con_from_hdl(client);

    // Figure out if the client knows the protocol.
    vector<string> p = connection->get_requested_subprotocols();
    if (std::find(p.begin(), p.end(), WS_PROTOCOL_NAME) != p.end()) {
        // Tell the client we're going to use this protocol
        connection->select_subprotocol(WS_PROTOCOL_NAME);
        return true;
    } else {
        return false;
    }
}
