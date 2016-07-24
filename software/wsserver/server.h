/**
 * SIGMusic Lights 2016
 * Socket server class
 */

#pragma once

#include <semaphore.h>
#include "common.h"
#include "transport.h"

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
    struct shared* s;
    struct threadargs {
        Transport* transport;
        int clientfd;
        void* _this;
    };

    void acceptLoop(Transport* t);
    static void* handleClient(void* arg);
    void processMessage(Transport* t, int clientfd, const std::string message);
};
