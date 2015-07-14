/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include <RF24/RF24.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "../network.h"

using websocketpp::connection_hdl;
using namespace websocketpp::frame;
using std::cout;
using std::endl;
using std::vector;
using std::string;

// Radio pins
#define CE_PIN                  RPI_BPLUS_GPIO_J8_22 // Radio chip enable pin
#define CSN_PIN                 RPI_BPLUS_GPIO_J8_24 // Radio chip select pin

// Wireless protocol
#define PING_TIMEOUT            10 // Time to wait for a ping response in milliseconds

// WebSocket protocol
#define WS_PROTOCOL_NAME        "nlcp" // Networked Lights Control Protocol

// Software information
#define VERSION                 0 // The software version number


typedef websocketpp::server<websocketpp::config::asio> server;

void initRadio(void);
void initWebSocket(void);
void pingAllLights(void);
void printWelcomeMessage(void);
void onMessage(connection_hdl hdl, server::message_ptr msg);
bool shouldConnect(connection_hdl hdl);


RF24 radio(CE_PIN, CSN_PIN);

uint8_t endpointID = BASE_STATION_ID;
bool lights[255] = {0};
server ws;


int main(int argc, char** argv){
    // Start the websocket
    initWebSocket();

    cout << "Running websocket event loop" << endl;
    // TODO: this I/O loop runs forever. Move to the bottom when done testing.
    ws.run();



    // Start the radio
    initRadio();

    printWelcomeMessage();

    radio.printDetails();



    cout << "Scanning for lights..." << endl;
    pingAllLights();
    cout << "Done scanning." << endl;


    // Broadcast red to all lights as a test
    packet_t red = {
        HEADER,
        CMD_SET_RGB,
        {255, 0, 0}
    };
    radio.stopListening();
    for (int i = 1; i <= 0xff; i++) {
        radio.openWritingPipe(RF_ADDRESS(i));
        radio.write(&red, sizeof(packet_t));
    }
    radio.startListening();

    return 0;
}

/**
 * Initializes the radio.
 */
void initRadio(void) {
    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setChannel(CHANNEL);
    radio.setPALevel(RF24_PA_MAX); // Range is important, not power consumption
    radio.setRetries(0, NUM_RETRIES);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.openReadingPipe(1, RF_ADDRESS(endpointID));
    radio.startListening();
}

/**
 * Initializes the websocket.
 */
void initWebSocket(void) {
    ws.set_message_handler(&onMessage);
    ws.set_validate_handler(&shouldConnect);

    ws.init_asio();
    ws.listen(7446);
    ws.start_accept();
}

/**
 * Pings every endpoint and records the ones that respond.
 */
void pingAllLights(void) {
    // Generate the ping packet
    packet_t ping = {
        HEADER,
        CMD_PING
    };

    for (int i = 0; i < 0xff; i++) {
        radio.stopListening();
        radio.openWritingPipe(RF_ADDRESS(i));
        bool success = radio.write(&ping, sizeof(packet_t));
        radio.startListening();

        // If the packet wasn't delivered after several attempts, move on
        if (!success) {
            continue;
        }
        
        // Wait here until we get a response or timeout
        unsigned int started_waiting_at = millis();
        bool timeout = false;
        while (!radio.available() && !timeout) {
            if ((millis() - started_waiting_at) > PING_TIMEOUT) {
                timeout = true;
            }
        }

        // Skip endpoints that timed out
        if (timeout) {
            continue;
        }

        // Get the response
        packet_t response;
        radio.read(&response, sizeof(packet_t));

        // Make sure the response checks out
        if (response.header != HEADER ||
            response.command != CMD_PING_RESPONSE ||
            response.data[0] != i) {
            continue;
        }

        printf("Found light 0x%02x\n", i);

        // Add the endpoint to the list
        lights[i] = true;
    }
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {
    puts("-------------------------------------");
    puts("SIGMusic@UIUC Lights Base Station");
    printf("Version %x\n", VERSION);
    puts("This base station is endpoint 0x00");
    puts("-------------------------------------");
}

/**
 * Handles incoming WebSocket messages.
 */
void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    cout << msg->get_payload() << endl;

    uint8_t id, r, g, b;

    const string message = msg->get_payload();
    if (!message.compare("list")) {
        // List
        ws.send(hdl, "TODO", opcode::text);

    } else if (!message.compare("discover")) {
        // Discover
        ws.send(hdl, "TODO", opcode::text);

    } else if (!message.compare(0, 7, "setrgb ")) {
        // Set RGB
        if (sscanf(message.c_str(), "setrgb %hhu %hhu %hhu %hhu", &id, &r, &g, &b) == 4) {
            // Arguments are valid
            printf("Setting light %u to %u, %u, %u\n", id, r, g, b);
            ws.send(hdl, "OK", opcode::text);

        } else {
            // One or more arguments were invalid
            ws.send(hdl, "Error: invalid arguments", opcode::text);
        }
    } else {
        // Error
        ws.send(hdl, "Error: unrecognized command", opcode::text);
    }
}

/**
 * Decides whether to connect to a client or not.
 */
bool shouldConnect(connection_hdl hdl) {
    // Get the connection so we can get info about it
    server::connection_ptr con = ws.get_con_from_hdl(hdl);

    // Figure out if the client knows the protocol.
    vector<string> p = con->get_requested_subprotocols();
    if (std::find(p.begin(), p.end(), WS_PROTOCOL_NAME) != p.end()) {
        // Tell the client we're going to use this protocol
        con->select_subprotocol(WS_PROTOCOL_NAME);
        return true;
    } else {
        return false;
    }
}

