/**
 * SIGMusic Lights 2016
 * Socket server class
 */

#include <string>
#include <iostream>
#include <sstream>
#include "server.h"
#include "common.h"
#include "wstransport.h"

using std::string;
using std::cout;
using std::endl;


Server::Server() {

}

void Server::run(struct shared* s) {

    this->s = s;

    Transport* ws = new WSTransport();

    int serverfd_ws = ws->init();

    while (1) {
        int clientfd = ws->accept(NULL, NULL);
        if (!fork()) {
            ws->close();
            handleClient(ws, clientfd);
            exit(EXIT_SUCCESS);
        }
    }
}

void Server::handleClient(Transport* t, int clientfd) {

    if (t->connect(clientfd)) {
        fprintf(stderr, "handshake failure\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected\n");

    char buf[2000];
    while (t->is_open(clientfd)) {

        int recvlen = t->recv(clientfd, buf, 2000);

        if (recvlen != -1) {
            std::string message = std::string(buf, recvlen);
            processMessage(t, clientfd, message);
        }
    }

    printf("Closed\n");
}

void Server::processMessage(Transport* t, int clientfd, const std::string message) {

    std::cout << message << std::endl;

    if (!message.compare(0, 4, "ping")) {
        // Echo the message - useful for measuring roundtrip latency
        t->send(clientfd, message.c_str(), message.length());

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
                snprintf(errorBuffer, sizeof(errorBuffer),
                    "Error: invalid arguments for ID %d", id);
                t->send(clientfd, errorBuffer, 50);
                break;
            }
            
            id++;
        }

        sem_post(&s->colors_sem);
    }
}
