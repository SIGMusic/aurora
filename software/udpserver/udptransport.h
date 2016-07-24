/**
 * SIGMusic Lights 2016
 * Connection-oriented UDP transport class
 */

#pragma once

#include "transport.h"

#include <map>
#include <vector>
#include <queue>
#include <utility>
#include <sys/socket.h>
#include <pthread.h>

class UDPTransport: public Transport {
public:
    UDPTransport();
    virtual ~UDPTransport();
    int init();
    int accept(struct sockaddr *addr, socklen_t *addrlen);
    int connect(int sockfd);
    int disconnect(int sockfd);
    ssize_t recv(int sockfd, char* buf, size_t len);
    ssize_t send(int sockfd, const char* buf, size_t len);
    int is_open(int sockfd);

private:
    typedef std::pair<std::vector<char>, uint16_t> connection_type;
    static std::map<connection_type, int> connections;
    static pthread_mutex_t con_mutex;

    static std::map<int, std::queue<std::vector<char>>> received_datagrams;
    static pthread_mutex_t dg_mutex;

    connection_type extract_address_port(struct sockaddr *sa);
};