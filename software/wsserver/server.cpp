/**
 * SIGMusic Lights 2016
 * Socket server class
 */

#include <string>
#include <iostream>
#include <sstream>
#include <pthread.h>
#include <sys/wait.h>
#include <sysexits.h>
 
#include <common.h>
#include "server.h"
#include "wstransport.h"

using std::string;
using std::cout;
using std::endl;


Server::Server() {

}

void Server::run(struct shared* s) {

    this->s = s;

    // Fork off the websocket transporter
    int ws_pid = fork();
    if (ws_pid == -1) {
        perror("fork");
        exit(EX_OSERR);
    } else if (ws_pid == 0) {
        Transport* ws = new WSTransport();
        acceptLoop(ws);
        exit(EXIT_FAILURE); // Should not get here
    }

    // Wait on all the children
    errno = 0;
    while (wait(NULL)) {
        if (errno == ECHILD) {
            break;
        }
    }
}

void Server::acceptLoop(Transport* t) {

    if (t->init() == -1) {
        exit(EXIT_FAILURE);
    }

    while (1) {
        
        int clientfd = t->accept(NULL, NULL);
        
        // Set up a struct to pass the parameters around
        struct threadargs* arg = new struct threadargs();
        arg->transport = t;
        arg->clientfd = clientfd;
        arg->_this = this;
        
        pthread_t pid;

        if (pthread_create(&pid, NULL, &Server::handleClient, arg)) {
            perror("pthread_create");
            continue;
        }

        if (pthread_detach(pid)) {
            perror("pthread_detach");
            continue;
        }
    }
}

void* Server::handleClient(void* arg) {

    // Ugly parameter passing to thread
    Transport* t = ((struct threadargs*)arg)->transport;
    int clientfd = ((struct threadargs*)arg)->clientfd;
    Server* _this = (Server*)((struct threadargs*)arg)->_this;
    delete (struct threadargs*)arg;

    if (t->connect(clientfd)) {
        fprintf(stderr, "handshake failure\n");
        return NULL;
    }

    printf("Connected\n");

    char buf[2000];
    while (t->is_open(clientfd)) {

        int recvlen = t->recv(clientfd, buf, 2000);

        if (recvlen != -1) {
            std::string message = std::string(buf, recvlen);
            _this->processMessage(t, clientfd, message);
        }
    }

    printf("Closed\n");
    return NULL;
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
