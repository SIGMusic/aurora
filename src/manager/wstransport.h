/**
 * SIGMusic Lights 2016
 * Websocket transport class
 */

#pragma once

#include "transport.h"

class WSTransport: public Transport {
public:
    WSTransport();
    virtual ~WSTransport();
    int init();
    int accept(struct sockaddr *addr, socklen_t *addrlen);
    int connect(int clientid);
    ssize_t recv(int clientid, char* buf, size_t len);
    ssize_t send(int clientid, const char* buf, size_t len);
    int close(int clientid);

private:
    int serverfd;
    ssize_t recv_complete(int sockfd, char*& buf);
    ssize_t send_complete(int sockfd, const char* buf, size_t len);
    char* base64(const unsigned char *input, int length);
};