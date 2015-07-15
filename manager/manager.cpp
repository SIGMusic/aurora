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
#define GET_TEMP_TIMEOUT        10 // Time to wait for a temperature response in milliseconds
#define GET_UPTIME_TIMEOUT      10 // Time to wait for an uptime response in milliseconds

// WebSocket protocol
#define WS_PROTOCOL_NAME        "nlcp" // Networked Lights Control Protocol
#define WS_PORT                 7446

typedef websocketpp::server<websocketpp::config::asio> server;

// Software information
#define VERSION                 0 // The software version number


void initRadio(void);
void initWebSocket(void);

bool waitForResponse(unsigned int timeout);
void pingAllLights(void);
bool getTemperature(uint8_t endpoint, int16_t * temp);
bool getUptime(uint8_t endpoint, uint16_t * uptime);
bool setRGB(uint8_t endpoint, uint8_t red, uint8_t green, uint8_t blue);

void printWelcomeMessage(void);

void onMessage(connection_hdl hdl, server::message_ptr msg);
bool shouldConnect(connection_hdl hdl);


RF24 radio(CE_PIN, CSN_PIN);

uint8_t endpointID = BASE_STATION_ID;
vector<bool> lights(256, false);
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

    // Disable verbose console output
    ws.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);
    ws.init_asio();
    ws.listen(WS_PORT);
    ws.start_accept();
}

/**
 * Blocks until a message is received or the timeout has passed.
 * @param timeout The time in milliseconds to wait for a response
 *
 * @return False if the timeout occurred, else true
 *
 * @todo Wait for a specific message format and allow others to be serviced
 *       in the mean time.
 */
bool waitForResponse(unsigned int timeout) {
    unsigned int started_waiting_at = millis();
    bool wasTimeout = false;
    while (!radio.available() && !wasTimeout) {
        if ((millis() - started_waiting_at) > timeout) {
            wasTimeout = true;
        }
    }
    return !wasTimeout;
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

    for (int i = 1; i <= 0xff; i++) {
        radio.stopListening();
        radio.openWritingPipe(RF_ADDRESS(i));
        bool success = radio.write(&ping, sizeof(packet_t));
        radio.startListening();

        // If the packet wasn't delivered after several attempts, move on
        if (!success) {
            lights[i] = false;
            continue;
        }
        
        // Wait here until we get a response or timeout
        if (!waitForResponse(PING_TIMEOUT)) {
            // Skip endpoints that timed out
            lights[i] = false;
            continue;
        }

        // Get the response
        packet_t response;
        radio.read(&response, sizeof(packet_t));

        // Make sure the response checks out
        if (response.header != HEADER ||
            response.command != CMD_PING_RESPONSE ||
            response.data[0] != i) {

            lights[i] = false;
            continue;
        }

        printf("Found light 0x%02x\n", i);

        // Add the endpoint to the list
        lights[i] = true;
    }
}

/**
 * Sets the RGB value of the given light.
 * @param endpoint The endpoint to change
 * @param red The new red value
 * @param green The new green value
 * @param blue The new blue value
 *
 * @return Whether the light acknowledged or not.
 */
bool setRGB(uint8_t endpoint, uint8_t red, uint8_t green, uint8_t blue) {
    printf("Setting light %u to %u, %u, %u\n", endpoint, red, green, blue);
    packet_t packet = {
        HEADER,
        CMD_SET_RGB,
        {red, green, blue}
    };

    radio.stopListening();
    radio.openWritingPipe(RF_ADDRESS(endpoint));
    bool worked = radio.write(&packet, sizeof(packet));
    radio.startListening();
    return worked;
}

/**
 * Gets the temperature of the given light.
 * @param endpoint The endpoint to change
 * @param temp The place to store the temperature in millidegrees Celsius
 *
 * @return Whether the light acknowledged or not.
 */
bool getTemperature(uint8_t endpoint, int16_t * temp) {
    packet_t request = {
        HEADER,
        CMD_GET_TEMP,
        {0, 0, 0}
    };

    radio.stopListening();
    radio.openWritingPipe(RF_ADDRESS(endpoint));
    bool worked = radio.write(&request, sizeof(request));
    radio.startListening();

    if (!worked) {
        // The light did not acknowledge
        return false;
    }

    if (!waitForResponse(GET_TEMP_TIMEOUT)) {
        // Did not get a response in time
        return false;
    }

    // Get the response
    packet_t response;
    radio.read(&response, sizeof(packet_t));

    // Make sure the response checks out
    if (response.header != HEADER ||
        response.command != CMD_TEMP_RESPONSE) {
        return false;
    }

    *temp = (response.data[0] << 8) | response.data[1];

    return true;
}

/**
 * Gets the uptime of the given light.
 * @param endpoint The endpoint to change
 * @param uptime The place to store the uptime in milliseconds
 *
 * @return Whether the light acknowledged or not.
 */
bool getUptime(uint8_t endpoint, uint16_t * uptime) {
    packet_t request = {
        HEADER,
        CMD_GET_UPTIME,
        {0, 0, 0}
    };

    radio.stopListening();
    radio.openWritingPipe(RF_ADDRESS(endpoint));
    bool worked = radio.write(&request, sizeof(request));
    radio.startListening();

    if (!worked) {
        // The light did not acknowledge
        return false;
    }

    if (!waitForResponse(GET_UPTIME_TIMEOUT)) {
        // Did not get a response in time
        return false;
    }

    // Get the response
    packet_t response;
    radio.read(&response, sizeof(packet_t));

    // Make sure the response checks out
    if (response.header != HEADER ||
        response.command != CMD_UPTIME_RESPONSE) {
        return false;
    }

    *uptime = (response.data[0] << 8) | response.data[1];

    return true;
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
    uint8_t id, r, g, b;

    const string message = msg->get_payload();
    if (!message.compare("list")) {
        // List
        string list = "";
        for (int i = 1; i <= 0xff; i++) {
            if (lights[i]) {
                char num[5];
                sprintf(num, "%d,", i);
                list.append(num);
            }
        }
        ws.send(hdl, list, opcode::text);

    } else if (!message.compare("discover")) {
        // Discover
        pingAllLights();
        ws.send(hdl, "OK", opcode::text);

    } else if (!message.compare(0, 7, "temp ")) {
        // Set RGB
        if (sscanf(message.c_str(), "temp %hhu", &id) == 1) {
            // Arguments are valid
            int16_t temp;
            if (getTemperature(id, &temp)) {
                // The light acknowledged
                ws.send(hdl, std::to_string(temp), opcode::text);
            } else {
                // The light did not acknowledge
                ws.send(hdl, "Error: light not responding", opcode::text);
            }

        } else {
            // One or more arguments were invalid
            ws.send(hdl, "Error: invalid arguments", opcode::text);
        }

    } else if (!message.compare(0, 7, "uptime ")) {
        // Set RGB
        if (sscanf(message.c_str(), "uptime %hhu", &id) == 1) {
            // Arguments are valid
            uint16_t uptime;
            if (getUptime(id, &uptime)) {
                // The light acknowledged
                ws.send(hdl, std::to_string(uptime), opcode::text);
            } else {
                // The light did not acknowledge
                ws.send(hdl, "Error: light not responding", opcode::text);
            }

        } else {
            // One or more arguments were invalid
            ws.send(hdl, "Error: invalid arguments", opcode::text);
        }

    } else if (!message.compare(0, 7, "setrgb ")) {
        // Set RGB
        if (sscanf(message.c_str(), "setrgb %hhu %hhu %hhu %hhu", &id, &r, &g, &b) == 4) {
            // Arguments are valid
            if (setRGB(id, r, g, b)) {
                // The light acknowledged
                ws.send(hdl, "OK", opcode::text);
            } else {
                // The light did not acknowledge
                ws.send(hdl, "Error: light not responding", opcode::text);
            }

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

