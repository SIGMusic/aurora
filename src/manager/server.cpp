/**
 * SIGMusic Lights 2016
 * Websocket server class
 */

#include <string>
#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include "server.h"
#include "common.h"


using websocketpp::connection_hdl;
using namespace websocketpp::frame;
using std::cout;
using std::endl;
using std::string;
using std::vector;


#define WS_PROTOCOL_NAME        "nlcp" // Networked Lights Control Protocol
#define WS_PORT                 7446


ws_server * Server::ws;

struct shared* Server::s;


Server::Server() {

    ws = new ws_server();
    ws->set_message_handler(&onMessage);
    ws->set_validate_handler(&shouldConnect);
}

void Server::run(struct shared* s) {
    
    Server::s = s;

    ws->init_asio();
    ws->listen(WS_PORT);
    ws->start_accept();

    ws->run();
}

void Server::onMessage(connection_hdl client, ws_server::message_ptr msg) {
    processMessage(client, msg->get_payload());
}

bool Server::shouldConnect(connection_hdl client) {

    // Get the connection so we can get info about it
    ws_server::connection_ptr connection = ws->get_con_from_hdl(client);

    // Figure out if the client knows the protocol
    vector<string> p = connection->get_requested_subprotocols();
    if (std::find(p.begin(), p.end(), WS_PROTOCOL_NAME) != p.end()) {
        // Tell the client we're going to use this protocol
        connection->select_subprotocol(WS_PROTOCOL_NAME);
        return true;
    } else {
        return false;
    }
}

void Server::processMessage(connection_hdl client, const std::string message) {

    if (!message.compare(0, 4, "ping")) {
        // Echo the message - useful for measuring roundtrip latency
        ws->send(client, message, opcode::text);

    } else {
        
        int id = 1;
        int r, g, b;
        std::istringstream stream(message);
        std::string token;

        sem_wait(&s->colors_sem);

        while (std::getline(stream, token, ',')) {

            if (sscanf(token.c_str(), "%02x%02x%02x", &r, &g, &b) == 3) {

                s->colors[id].r = (uint8_t)r;
                s->colors[id].g = (uint8_t)g;
                s->colors[id].b = (uint8_t)b;

            } else {

                // One or more arguments were invalid
                char errorBuffer[50];
                sprintf(errorBuffer, "Error: invalid arguments for ID %d", id);
                ws->send(client, errorBuffer, opcode::text);
                break;
            }
            
            id++;
        }

        sem_post(&s->colors_sem);
    }
}
