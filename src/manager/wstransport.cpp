/**
 * SIGMusic Lights 2016
 * Websocket transport class
 */

#include "wstransport.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>


#define WS_PROTOCOL_NAME    "nlcp" // the subprotocol that clients must support:
                                   // Networked Lights Control Protocol
#define WS_PORT             "7446" // the port users will be connecting to
#define BACKLOG             10 // how many pending connections queue will hold


WSTransport::WSTransport(): Transport() {

}

WSTransport::~WSTransport() {

}

int WSTransport::init() {

    // Adapted from Beej's Guide
    struct addrinfo hints, *servinfo, *p;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, WS_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((serverfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            freeaddrinfo(servinfo);
            return -1;
        }

        if (bind(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(serverfd);
            perror("bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }

    if (listen(serverfd, BACKLOG) == -1) {
        perror("listen");
        return -1;
    }

    return 0;
}

int WSTransport::accept(struct sockaddr *addr, socklen_t *addrlen) {

    return ::accept(serverfd, addr, addrlen);
}

int WSTransport::connect(int clientid) {

    int bad_request = 0;

    // Get the request string
    char* buf = NULL;
    int recvlen = recv_complete_headers(clientid, buf);
    if (recvlen == -1) {
        perror("recv");
        bad_request = 1;
    }

    // Verify the GET request is valid
    unsigned int version_major = 0;
    unsigned int version_minor = 0;
    if (sscanf(buf, "GET / HTTP/%u.%u\r\n", &version_major, &version_minor) != 2) {
        fprintf(stderr, "WSTransport::connect: failed to parse request\n");
        bad_request = 1;
    }

    // Verify the version number is 1.1 or greater
    if (version_major < 1 ||
        (version_major == 1 && version_minor < 1)) {
        fprintf(stderr, "WSTransport::connect: HTTP version < 1.1\n");
        bad_request = 1;
    }

    // Verify the basic required headers are present
    if (strstr(buf, "\r\nHost: ") == NULL) {
        fprintf(stderr, "WSTransport::connect: missing Host\n");
        bad_request = 1;
    }

    if (strstr(buf, "\r\nUpgrade: websocket\r\n") == NULL) {
        fprintf(stderr, "WSTransport::connect: missing Upgrade: websocket\n");
        bad_request = 1;
    }

    if (strstr(buf, "\r\nConnection: Upgrade\r\n") == NULL) {
        fprintf(stderr, "WSTransport::connect: missing Connection: Upgrade\n");
        bad_request = 1;
    }

    if (strstr(buf, "\r\nSec-WebSocket-Version: 13\r\n") == NULL) {
        fprintf(stderr, "WSTransport::connect: missing Sec-WebSocket-Version: 13\n");
        bad_request = 1;
    }

    // Verify the protocol matches
    char* protocol_header = strstr(buf, "\r\nSec-WebSocket-Protocol: ");

    if (protocol_header == NULL) {
        fprintf(stderr, "WSTransport::connect: missing Sec-WebSocket-Protocol\n");
        bad_request = 1;
    } else {

        char protocol[5];
        if (sscanf(protocol_header, "\r\nSec-WebSocket-Protocol: %4s\r\n", protocol) != 1) {
            fprintf(stderr, "WSTransport::connect: failed to parse Sec-WebSocket-Protocol\n");
            bad_request = 1;
        } else if (strcmp(protocol, WS_PROTOCOL_NAME)) {
            fprintf(stderr, "WSTransport::connect: incorrect Sec-WebSocket-Protocol value\n");
            bad_request = 1;
        }
    }

    // Verify the key is provided and extract it
    char* key_header = strstr(buf, "\r\nSec-WebSocket-Key: ");
    char key[25]; // Key is a 16 byte number, base64-encoded = 24 bytes

    if (key_header == NULL) {
        fprintf(stderr, "WSTransport::connect: missing Sec-WebSocket-Key\n");
        bad_request = 1;
    } else {
        if (sscanf(key_header, "\r\nSec-WebSocket-Key: %24s\r\n", key) != 1) {
            fprintf(stderr, "WSTransport::connect: failed to parse Sec-WebSocket-Key\n");
            bad_request = 1;
        }
    }

    // Done reading the request
    free(buf);

    if (!bad_request) {

        // The request was valid. Finish the handshake.

        // Calculate the response key
        char magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        char newkey[sizeof(key) + sizeof(magic)];
        // snprintf(newkey, sizeof(newkey), "%s%s", key, magic);
        strcpy(newkey, key);
        strcat(newkey, magic);
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1((unsigned char*)newkey, strlen(newkey), hash);
        char* accept_token = base64(hash, sizeof(hash));

        // Send the response
        char response_buf[200];
        snprintf(response_buf, sizeof(response_buf),
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Protocol: %4s\r\n"
            "Sec-WebSocket-Accept: %s\r\n"
            "\r\n",
            WS_PROTOCOL_NAME, accept_token);

        free(accept_token);

        if (send_complete(clientid, response_buf, strlen(response_buf)) == -1) {
            return -1;
        }

        return 0;

    } else {

        // The request was invalid. Reject and close the connection.

        char response_buf[100];
        sprintf(response_buf,
            "HTTP/1.1 400 Bad Request\r\n"
            "Connection: Closed\r\n"
            "\r\n"
            "<h1>400 Bad Request</h1>");

        send_complete(clientid, response_buf, strlen(response_buf));

        close(clientid);

        return -1;
    }
}

ssize_t WSTransport::recv(int clientid, char* buf, size_t len) {

    int recvlen = recv_complete_msg(clientid, buf, len);
    if (recvlen == -1) {
        perror("recv");
        return -1;
    }

    // TODO: parse frame and extract payload
    // Rebuild the payload if it is broken up

    return recvlen;
}

ssize_t WSTransport::send(int clientid, const char* buf, size_t len) {

    return 0; // TODO
}

int WSTransport::close(int clientid) {

    // TODO
    return ::close(clientid);
}

ssize_t WSTransport::recv_complete_headers(int sockfd, char*& buf) {

    int received = 0;

    char recvbuf[1000];
    recvbuf[0] = '\0';

    buf = (char*)malloc(100);
    buf[0] = '\0';

    while (!strstr(recvbuf, "\r\n\r\n")) {

        int rv = ::recv(sockfd, recvbuf, sizeof(recvbuf) - 1, 0);

        if (rv == -1) {
            perror("recv");
            return -1;
        }

        recvbuf[rv] = '\0';

        received += rv;

        buf = (char*)realloc(buf, received + 1);
        strncat(buf, recvbuf, rv);
    }

    return received;
}

ssize_t WSTransport::recv_complete_msg(int sockfd, char*& buf, size_t len) {

    return 0; // TODO
}

ssize_t WSTransport::send_complete(int sockfd, const char* buf, size_t len) {

    int sent = 0;
    while (sent < len) {

        int rv = ::send(sockfd, buf + sent, len - sent, 0);

        if (rv == -1) {
            perror("send");
            return -1;
        }

        sent += rv;
    }

    return sent;
}

char* WSTransport::base64(const unsigned char *input, int len) {

    // Adapted from http://www.ioncannon.net/programming/34/howto-base64-encode-with-cc-and-openssl/
    
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    if (BIO_write(b64, input, len) < 1) {
        fprintf(stderr, "BIO_write: failed\n");
        return NULL;
    }

    if (BIO_flush(b64) != 1) {
        fprintf(stderr, "BIO_flush: failed\n");
        return NULL;
    }

    BIO_get_mem_ptr(b64, &bptr);

    // Original code allocated bptr->length, but this was frequently -1.
    // Since in this case the encoded value will always be 28 bytes long
    // (plus null terminator), just allocate that.
    char *buf = (char*)malloc(29);
    memcpy(buf, bptr->data, 28);
    buf[28] = '\0';

    BIO_free_all(b64);

    return buf;
}