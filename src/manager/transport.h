/**
 * SIGMusic Lights 2016
 * Transport layer abstract class
 */

#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

class Transport {
public:

    /**
     * Constructs a transporter.
     */
    Transport() { }

    /**
     * Destroys a transporter. Stops listening for new connections.
     */
    virtual ~Transport() { }

    /**
     * Initializes a transporter and begins listening for connections.
     *
     * @return     the server's socket on success, or -1 on error
     */
    virtual int init() = 0;

    /**
     * Waits for and accepts an incoming connection from a client.
     *
     * @param      addr     The client's address structure
     * @param      addrlen  The length of the client's address structure
     *
     * @return     the socket for the new connection, or -1 on error
     */
    virtual int accept(struct sockaddr *addr, socklen_t *addrlen) = 0;

    /**
     * Establishes the connection by performing any necessary handshaking.
     *
     * @param[in]  sockfd    The client's socket
     *
     * @return     0 on success, or -1 on error
     */
    virtual int connect(int sockfd) = 0;

    /**
     * Closes the connection normally by sending any necessary closing messages.
     *
     * @param[in]  sockfd    The client's socket
     *
     * @return     0 on success, or -1 on error
     */
    virtual int disconnect(int sockfd) = 0;

    /**
     * Waits for and receives a message from the client.
     *
     * @param[in]  sockfd    The client's socket
     * @param      buf       The buffer to write the data into
     * @param[in]  len       The number of bytes to try to receive
     *
     * @return     the number of bytes received, or -1 on error
     */
    virtual ssize_t recv(int sockfd, char* buf, size_t len) = 0;

    /**
     * Sends a message to the client.
     *
     * @param[in]  sockfd    The client's socket
     * @param[in]  buf       The buffer containing the data to send
     * @param[in]  len       The number of bytes to send from buf
     *
     * @return     the number of bytes sent, or -1 on error
     */
    virtual ssize_t send(int sockfd, const char* buf, size_t len) = 0;

    /**
     * Closes the server socket.
     *
     * @return     0 on success, or -1 on error
     */
    virtual int close() {
        return ::close(serverfd);
    }

    /**
     * Checks if the socket is open
     *
     * @return     nonzero if open, 0 if closed
     */
    virtual int is_open(int sockfd) {
        int err;
        socklen_t len = sizeof(err);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len)) {
            return 0;
        } else {
            return 1;
        }
    }

protected:
    int serverfd;
};
