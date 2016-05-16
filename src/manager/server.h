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
    
    void handleClient(Transport* t, int clientfd);
    void processMessage(Transport* t, int clientfd, const std::string message);
    struct shared* s;
};
