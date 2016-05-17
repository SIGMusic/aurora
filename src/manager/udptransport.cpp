/**
 * SIGMusic Lights 2016
 * Connection-oriented UDP transport class
 */

#include "udptransport.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <algorithm>


#define PORT             "7447" // the port users will be connecting to


// Maps <client_addr, client_port> to fake_sockfd
std::map<UDPTransport::connection_type, int> UDPTransport::connections;

// Maps fake_sockfd to a vector of all pending incoming datagrams
std::map<int, std::queue<std::vector<char>>> UDPTransport::received_datagrams;


UDPTransport::UDPTransport(): Transport() {

}

UDPTransport::~UDPTransport() {

}

int UDPTransport::init() {

    // Adapted from Beej's Guide
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "UDPTransport: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((serverfd = ::socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("UDPTransport: socket");
            continue;
        }

        if (::bind(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
            ::close(serverfd);
            perror("UDPTransport: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "UDPTransport: failed to bind\n");
        return -1;
    }

    freeaddrinfo(servinfo);

    return serverfd;
}

int UDPTransport::accept(struct sockaddr *addr, socklen_t *addrlen) {

    // 1023 is the default max file descriptor per process;
    // using values outside the valid range provides a safeguard in case
    // the artificial client ID gets passed to a syscall as a file descriptor
    static int next_fake_fd = 1024;
    
    // Wait for anyone to send us anything
    connection_type id;
    struct sockaddr_storage my_addr;
    socklen_t my_addrlen = sizeof(my_addr);
    do {
        char c;

        while (recvfrom(serverfd, &c, sizeof(c), MSG_PEEK,
                (struct sockaddr*)&my_addr, &my_addrlen) != sizeof(c));

        id = extract_address_port((struct sockaddr*)&my_addr);

    } while (connections.find(id) != connections.end()); // Ignore existing clients

    // Add the entry to the table
    connections[id] = next_fake_fd;

    if (addr != NULL) {
        memcpy(addr, &my_addr, *addrlen);
        *addrlen = my_addrlen;
    }

    return next_fake_fd++;
}

int UDPTransport::connect(int sockfd) {

    // TODO: add opening handshake
    
    if (is_open(sockfd)) {
        return 0;
    } else {
        return -1;
    }
}

int UDPTransport::disconnect(int sockfd) {
    
    // TODO: add closing handshake

    // Remove any outstanding received datagrams
    received_datagrams.erase(sockfd);

    // Remove it from the connections
    for (auto it = connections.begin(); it != connections.end(); it++) {
        if (it->second == sockfd) {
            connections.erase(it);
            return 0;
        }
    }

    // Couldn't find any matching connection
    return -1;
}

ssize_t UDPTransport::recv(int sockfd, char* buf, size_t len) {

    ssize_t received;

    // Check whether any existing messages are available
    if (!received_datagrams[sockfd].empty()) {
        std::vector<char> dg = received_datagrams[sockfd].front();
        received_datagrams[sockfd].pop();
        std::copy(dg.begin(), dg.end(), buf);
        return dg.size();
    }

    while (1) {

        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);

        received = recvfrom(serverfd, buf, len, 0, &addr, &addrlen);
        if (received == -1) {
            perror("UDPTransport: recvfrom");
            return -1;
        }

        // Figure out if the connection is for the right address or not
        connection_type id = extract_address_port(&addr);
        auto it = connections.find(id);
        if (it == connections.end()) {
            // Ignore message from unconnected client
            continue;
        }

        int fd = it->second;
        if (fd == sockfd) {
            // The datagram was for this client, so return it
            break;
        } else {
            // The datagram was for a different client, so store it
            std::vector<char> dg(buf, buf + received);
            received_datagrams[fd].push(dg);
        }
    }

    return received;
}

ssize_t UDPTransport::send(int sockfd, const char* buf, size_t len) {

    connection_type id;
    bool found = false;

    for (auto it = connections.begin(); it != connections.end(); it++) {
        if (it->second == sockfd) {
            id = it->first;
            found = true;
            break;
        }
    }

    if (!found) {
        // Client is not connected
        return -1;
    }

    // Rebuild the sockaddr struct
    struct sockaddr_storage addr;
    socklen_t addrlen;

    std::vector<char> addr_vec = id.first;
    uint16_t addr_port = id.second;

    if (addr_vec.size() == 4) { // IPv4
        
        addr.ss_family = AF_INET;

        ((struct sockaddr_in*)&addr)->sin_port = htons(addr_port);

        std::copy(addr_vec.begin(), addr_vec.end(),
            (char*)&(((struct sockaddr_in*)&addr)->sin_addr));

        memset((char*)&(((struct sockaddr_in*)&addr)->sin_zero), 0, 8);
        
        addrlen = sizeof(struct sockaddr_in);

    } else { // IPv6
        
        addr.ss_family = AF_INET6;
        
        ((struct sockaddr_in6*)&addr)->sin6_port = htons(addr_port);

        std::copy(addr_vec.begin(), addr_vec.end(),
            (char*)&(((struct sockaddr_in6*)&addr)->sin6_addr));
        
        addrlen = sizeof(struct sockaddr_in6);
    }

    return sendto(serverfd, buf, len, 0, (struct sockaddr*)&addr, addrlen);
}

int UDPTransport::is_open(int sockfd) {

    for (auto it = connections.begin(); it != connections.end(); it++) {
        if (it->second == sockfd) {
            return 1;
        }
    }

    return 0;
}

/**
 * Extracts the IP address and port information from the sockaddr struct.
 *
 * @param      sa       The sockaddr struct
 *
 * @return     A pair of <the address as a char vector, the port as a uint16>
 */
UDPTransport::connection_type UDPTransport::extract_address_port(struct sockaddr *sa) {

    std::vector<char> addr;
    uint16_t port;

    if (sa->sa_family == AF_INET) { // IPv4

        port = ntohs(((struct sockaddr_in*)sa)->sin_port);

        char* sin_addr = (char*)&(((struct sockaddr_in*)sa)->sin_addr);
        addr.assign(sin_addr, sin_addr + 4);

    } else { // IPv6
        
        port = ntohs(((struct sockaddr_in6*)sa)->sin6_port);

        char* sin6_addr = (char*)&(((struct sockaddr_in6*)sa)->sin6_addr);
        addr.assign(sin6_addr, sin6_addr + 16);
    }

    return std::make_pair(addr, port);
}