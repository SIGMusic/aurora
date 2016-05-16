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
    int connect(int sockfd);
    int disconnect(int sockfd);
    ssize_t recv(int sockfd, char* buf, size_t len);
    ssize_t send(int sockfd, const char* buf, size_t len);

private:
    ssize_t recv_headers(int sockfd, char*& buf);
    ssize_t recv_frame(int sockfd, char* buf, size_t len);
    ssize_t recv_complete(int sockfd, char* buf, size_t len);
    ssize_t send_complete(int sockfd, const char* buf, size_t len);
    int handle_control_frame(int sockfd, char* buf, size_t len);
    int disconnect(int sockfd, int status);
    char* base64(const unsigned char *input, int len);
};