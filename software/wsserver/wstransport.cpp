/**
 * SIGMusic Lights 2016
 * Websocket transport class
 */

#include "wstransport.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

// Add support for htonll and ntohll if necessary
#ifndef htonll
#define htonll(x) htobe64(x)
#endif
#ifndef ntohll
#define ntohll(x) be64toh(x)
#endif


#define PROTOCOL_NAME    "nlcp" // the subprotocol that clients must support:
                                // Networked Lights Control Protocol
#define PORT             "7446" // the port users will be connecting to
#define BACKLOG             10 // how many pending connections queue will hold

enum CLOSING_CODES {
    CLOSE_NORMAL = 1000,
    CLOSE_GOING_AWAY,
    CLOSE_PROTOCOL_ERROR,
    CLOSE_UNSUPPORTED,
    CLOSE_NO_STATUS = 1005,
    CLOSE_ABNORMAL,
    CLOSE_INVALID_PAYLOAD,
    CLOSE_POLICY_VIOLATION,
    CLOSE_TOO_LARGE,
    CLOSE_EXTENSION_REQUIRED,
    CLOSE_INTERNAL_SERVER_ERROR,
    CLOSE_RSVD_TLS_HANDSHAKE = 1015,
};

enum OPCODES {
    OP_CONTINUATION = 0x0,
    OP_TEXT,
    OP_BINARY,
    OP_CLOSE = 0x8,
    OP_PING,
    OP_PONG,
};


WSTransport::WSTransport(): Transport() {

}

WSTransport::~WSTransport() {

}

int WSTransport::init() {

    // Adapted from Beej's Guide
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "WSTransport: getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((serverfd = ::socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("WSTransport: socket");
            continue;
        }

        if (::bind(serverfd, p->ai_addr, p->ai_addrlen) == -1) {
            ::close(serverfd);
            perror("WSTransport: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "WSTransport: failed to bind\n");
        return -1;
    }

    if (::listen(serverfd, BACKLOG) == -1) {
        perror("WSTransport: listen");
        return -1;
    }

    return serverfd;
}

int WSTransport::accept(struct sockaddr *addr, socklen_t *addrlen) {

    int fd = ::accept(serverfd, addr, addrlen);
    if (fd == -1) {
        perror("WSTransport: accept");
        return -1;
    }

    return fd;
}

int WSTransport::connect(int sockfd) {

    int bad_request = 0;
    int incorrect_version = 0;

    // Get the request string
    char* buf = NULL;
    int recvlen = recv_headers(sockfd, buf);
    if (recvlen == -1) {
        perror("WSTransport: recv");
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

        // Check if an incorrect version is specified so we can tell the client
        // what version(s) we do support
        if (strstr(buf, "\r\nSec-WebSocket-Version: ") != NULL) {
            incorrect_version = 1;
        }
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
        } else if (strcmp(protocol, PROTOCOL_NAME)) {
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
            PROTOCOL_NAME, accept_token);

        free(accept_token);

        if (send_complete(sockfd, response_buf, strlen(response_buf)) == -1) {
            return -1;
        }

        return 0;

    } else {

        // The request was invalid. Reject the handshake,
        // but do not close the connection.

        // Send a version header if the version number was incorrect
        char version_header[40];
        version_header[0] = '\0';
        if (incorrect_version) {
            strcpy(version_header, "Sec-WebSocket-Version: 13\r\n");
        }

        char response_buf[100];
        sprintf(response_buf,
            "HTTP/1.1 400 Bad Request\r\n"
            "%s"
            "\r\n"
            "<h1>400 Bad Request</h1>",
            version_header);

        send_complete(sockfd, response_buf, strlen(response_buf));

        return -1;
    }
}

int WSTransport::disconnect(int sockfd) {
    return disconnect(sockfd, CLOSE_NORMAL);
}

/**
 * Sends a closing frame with the given status.
 *
 * @param[in]  sockfd  The socket to send to
 * @param[in]  status  The status code to provide
 *
 * @return     0 on success, or -1 on failure
 */
int WSTransport::disconnect(int sockfd, int status) {

    char frame[4];

    frame[0] = 0b10000000 | OP_CLOSE;
    frame[1] = 2;
    frame[2] = (htons(status) >> 8) & 0xFF;
    frame[3] = (htons(status) >> 0) & 0xFF;

    if (send_complete(sockfd, frame, sizeof(frame)) == -1) {
        return -1;
    }

    // Wait for response
    int opcode = 0;
    do {
        char buf[1];
        if (recv_complete(sockfd, buf, 1) == -1) {
            return -1;
        }

        opcode = buf[0] & 0b00001111;
        
    } while (opcode != OP_CLOSE);

    return 0;
}

ssize_t WSTransport::recv(int sockfd, char* buf, size_t len) {

    size_t received = 0;
    int num_frames_processed = 0;

    while (received < len) {

        char framebuf[len];
        ssize_t framelen = recv_frame(sockfd, framebuf, len);
        
        if (framelen == -1) {
            return -1;
        }

        if (framelen < 2) {
            continue;
        }

        char* p = framebuf;
        int fin    = *p & 0b10000000;
        int rsv    = *p & 0b01110000;
        int opcode = *p & 0b00001111;
        p++;

        if (rsv != 0) {
            fprintf(stderr, "WSTransport::recv_message: RSV bits were not 0\n");
            disconnect(sockfd, CLOSE_PROTOCOL_ERROR);
            return -1;
        }

        if (opcode & 0b1000) {
            // A control frame was received
            if (handle_control_frame(sockfd, framebuf, framelen) == 1) {
                // Socket closed in the middle of receiving.
                // Discard the message.
                return -1;
            }
            continue;
        } else if (opcode == OP_CONTINUATION && num_frames_processed == 0) {
            fprintf(stderr, "WSTransport::recv_message: received continuation frame with no initial frame\n");
            disconnect(sockfd, CLOSE_PROTOCOL_ERROR);
            return -1;
        } else if (opcode == OP_TEXT && num_frames_processed != 0) {
            fprintf(stderr, "WSTransport::recv_message: received duplicate initial frame\n");
            disconnect(sockfd, CLOSE_PROTOCOL_ERROR);
            return -1;
        } else if (opcode != OP_TEXT && opcode != OP_CONTINUATION) {
            fprintf(stderr, "WSTransport::recv_message: unsupported data type\n");
            disconnect(sockfd, CLOSE_UNSUPPORTED);
            return -1;
        }

        int mask_bit       = *p & 0b10000000;
        size_t payload_len = *p & 0b01111111;
        p++;

        if (payload_len == 126) {
            payload_len = ntohs(*(uint16_t*)p);
            p += sizeof(uint16_t);
        } else if (payload_len == 127) {
            payload_len = ntohll(*(uint64_t*)p);
            p += sizeof(uint64_t);
        }

        char mask[4];
        if (mask_bit) {
            memcpy(mask, p, sizeof(uint32_t));
            p += sizeof(uint32_t);
        } else {
            fprintf(stderr, "WSTransport::recv_message: mask bit was not set\n");
            disconnect(sockfd, CLOSE_PROTOCOL_ERROR);
            return -1;
        }

        size_t bytes_left = len - received;

        for (size_t i = 0; i < std::min(payload_len, bytes_left); i++) {
            buf[received + i] = p[i] ^ mask[i % 4];
        }

        received += std::min(payload_len, bytes_left);
        num_frames_processed++;

        if (fin) {
            break;
        }
    }

    return received;
}

ssize_t WSTransport::send(int sockfd, const char* buf, size_t len) {

    ssize_t sent = 0;

    char header[10];
    
    header[0] = 0b10000000 | OP_TEXT;

    // Calculate the payload length field
    ssize_t headerlen = 1;
    if (len < 126) {

        header[1] = len;
        headerlen += 1;

    } else if (len <= 0xFFFF) {

        header[1] = 126;
        header[2] = (htons(len) >> 8) & 0xFF;
        header[3] = (htons(len) >> 0) & 0xFF;
        headerlen += 1 + sizeof(uint16_t);

    } else if (len <= 0x7FFFFFFFFFFFFFFF) {

        header[1] = 127;
        header[2] = (htonll(len) >> 56) & 0xFF;
        header[3] = (htonll(len) >> 48) & 0xFF;
        header[4] = (htonll(len) >> 40) & 0xFF;
        header[5] = (htonll(len) >> 32) & 0xFF;
        header[6] = (htonll(len) >> 24) & 0xFF;
        header[7] = (htonll(len) >> 16) & 0xFF;
        header[8] = (htonll(len) >> 8)  & 0xFF;
        header[9] = (htonll(len) >> 0)  & 0xFF;
        headerlen += 1 + sizeof(uint64_t);

    } else {
        // Too long
        return -1;
    }

    // Send the header
    ssize_t rv = send_complete(sockfd, header, headerlen);
    if (rv != headerlen) {
        return -1;
    }
    sent += rv;

    // Send the payload
    rv = send_complete(sockfd, buf, len);
    if (rv != (ssize_t)len) {
        return -1;
    }
    sent += rv;

    return sent;
}

/**
 * Receives HTTP headers up to the terminating \r\n\r\n.
 *
 * @param[in]  sockfd  The socket to receive from
 * @param      buf     A reference that will be filled in with the allocated
 *                     buffer. This buffer must be freed by the caller.
 *
 * @return     the number of bytes received, or -1 on error
 */
ssize_t WSTransport::recv_headers(int sockfd, char*& buf) {

    ssize_t received = 0;

    char recvbuf[1000];
    recvbuf[0] = '\0';

    buf = (char*)malloc(100);
    buf[0] = '\0';

    while (!strstr(recvbuf, "\r\n\r\n")) {

        ssize_t rv = ::recv(sockfd, recvbuf, sizeof(recvbuf) - 1, 0);

        if (rv == -1) {
            perror("WSTransport: recv");
            return -1;
        }

        recvbuf[rv] = '\0';

        received += rv;

        buf = (char*)realloc(buf, received + 1);
        strncat(buf, recvbuf, rv);
    }

    return received;
}

/**
 * Receives an entire websocket frame.
 *
 * @param[in]  sockfd  The socket to receive from
 * @param      buf     The buffer to contain the frame
 * @param[in]  len     The maximum number of bytes to write
 *
 * @return     the number of bytes written, or -1 on error
 */
ssize_t WSTransport::recv_frame(int sockfd, char* buf, size_t len) {

    ssize_t recvlen;
    ssize_t received = 0;

    // Don't bother if the frame is smaller than the minimal header
    if (len < 2) {
        return 0;
    }

    // Receive up to the payload length
    recvlen = recv_complete(sockfd, buf, 2);
    if (recvlen == -1) {
        return -1;
    }
    received += recvlen;
    len -= recvlen;

    char* p = buf;
    p++;
    int mask_bit       = *p & 0b10000000;
    size_t payload_len = *p & 0b01111111;
    p++;

    if (payload_len == 126) {

        // Receive the 16-bit extended payload length
        recvlen = recv_complete(sockfd, p, std::min(sizeof(uint16_t), len));
        if (recvlen == -1) {
            return -1;
        }
        received += recvlen;
        len -= recvlen;

        payload_len = ntohs(*(uint16_t*)p);
        p += sizeof(uint16_t);

    } else if (payload_len == 127) {

        // Receive the 64-bit extended payload length
        recvlen = recv_complete(sockfd, p, std::min(sizeof(uint64_t), len));
        if (recvlen == -1) {
            return -1;
        }
        received += recvlen;
        len -= recvlen;

        payload_len = ntohll(*(uint64_t*)p);
        p += sizeof(uint16_t);
    }

    if (mask_bit) {

        // Receive the masking key
        recvlen = recv_complete(sockfd, p, std::min(sizeof(uint32_t), len));
        if (recvlen == -1) {
            return -1;
        }
        received += recvlen;
        len -= recvlen;

        p += sizeof(uint32_t);
    }

    // Receive the payload
    recvlen = recv_complete(sockfd, p, std::min(payload_len, len));
    if (recvlen == -1) {
        return -1;
    }
    received += recvlen;
    len -= recvlen;

    return received;
}

/**
 * Receives the number of bytes requested, not less.
 *
 * @param[in]  sockfd  The socket to receive from
 * @param      buf     The buffer to write to
 * @param[in]  len     The number of bytes to receive
 *
 * @return     the number of bytes written, or -1 on error
 */
ssize_t WSTransport::recv_complete(int sockfd, char* buf, size_t len) {

    size_t received = 0;
    while (received < len) {

        ssize_t rv = ::recv(sockfd, buf + received, len - received, 0);

        if (rv == -1) {
            perror("WSTransport: recv");
            return -1;
        }

        received += rv;
    }

    return received;
}

/**
 * Sends the number of bytes requested, not less.
 *
 * @param[in]  sockfd  The socket to send to
 * @param      buf     The buffer to read from
 * @param[in]  len     The number of bytes to send
 *
 * @return     the number of bytes sent, or -1 on error
 */
ssize_t WSTransport::send_complete(int sockfd, const char* buf, size_t len) {

    size_t sent = 0;
    while (sent < len) {

        ssize_t rv = ::send(sockfd, buf + sent, len - sent, 0);

        if (rv == -1) {
            perror("WSTransport: send");
            return -1;
        }

        sent += rv;
    }

    return sent;
}

/**
 * Handles any incoming control frames.
 *
 * @param[in]  sockfd  The socket to reply to if necessary
 * @param[in]  buf     The buffer containing the frame
 * @param[in]  len     The length of the frame
 *
 * @return     0 on success, 1 on socket close, or -1 on error
 */
int WSTransport::handle_control_frame(int sockfd, char* buf, size_t len) {

    // Check the opcode
    int opcode = buf[0] & 0b00001111;

    switch (opcode) {

    case OP_CLOSE:
        // Respond with matching close message
        buf[1] = 0;
        if (send_complete(sockfd, buf, 2) == -1) {
            return -1;
        }
        ::close(sockfd);
        return 1;

    case OP_PING:
        // Respond with matching pong message
        buf[0] &= ~0b00001111;
        buf[0] |= OP_PONG;
        if (send_complete(sockfd, buf, len) == -1) {
            return -1;
        }
        break;

    case OP_PONG:
        // Ignore unsolicited pong
        break;

    default:
        // Not a recognized control frame
        return -1;
    }

    return 0;
}

/**
 * Encodes an input array as base64.
 *
 * @param[in]  input  The input data to encode
 * @param[in]  len    The length of the data to encode
 *
 * @return     a newly allocated string containing the encoded data
 */
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