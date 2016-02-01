/**
 * SIGMusic Lights 2015
 * Raspberry Pi manager
 */

#include <cstdlib>
#include <iostream>
#include <sys/mman.h>
#include <semaphore.h>
#include <websocketpp/server.hpp>
#include "server.h"
#include "radio.h"
#include "manager.h"

using websocketpp::connection_hdl;
using std::cout;
using std::endl;
using std::vector;
using std::string;


// Software information
#define VERSION                 0 // The software version number

// Wireless protocol
#define PING_TIMEOUT            10 // Time to wait for a ping response in milliseconds
#define GET_TEMP_TIMEOUT        10 // Time to wait for a temperature response in milliseconds
#define GET_UPTIME_TIMEOUT      10 // Time to wait for an uptime response in milliseconds


void pingAllLights(void);
bool getTemperature(uint8_t endpoint, int16_t * temp);
bool getUptime(uint8_t endpoint, uint16_t * uptime);
bool setRGB(uint8_t endpoint, uint8_t red, uint8_t green, uint8_t blue);
void processMessage(connection_hdl client, const string message);
void printWelcomeMessage(void);


Server server;
Radio radio;
vector<bool> connected(NUM_IDS, false);
color_t* colors;
sem_t* colors_sem;


int main(int argc, char** argv) {
    
    printWelcomeMessage();

    // Map a shared array to store the colors
    colors = (color_t*)mmap(NULL, NUM_IDS * sizeof(color_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (colors == (void*)-1) {
        perror("mmap");
        exit(1);
    }

    // Set up the colors semaphore
    colors_sem = (sem_t*)mmap(NULL, sizeof(sem_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (colors_sem == (void*)-1) {
        perror("mmap");
        exit(1);
    }

    if (sem_init(colors_sem, 1, 1)) {
        perror("sem_init");
        exit(1);
    }

    // Fork off the websocket server
    if (!fork()) {

        cout << "Running websocket event loop" << endl;
        server.run(); // Runs forever
        
        // Should never get here
        return 0;
    }

    // Fork off the network controller
    if (!fork()) {

        cout << "Scanning for lights..." << endl;
        pingAllLights();
        cout << "Done scanning." << endl;

        clock_t last_update = clock();
        while(1) {
            
            // Only transmit once per frame
            clock_t now = clock();
            if ((((float)now)/CLOCKS_PER_SEC -
                ((float)last_update)/CLOCKS_PER_SEC) > 1.0/MAX_FPS) {

                sem_wait(colors_sem);
                for (int i = 1; i < NUM_IDS; i++) {
                    
                    // Only transmit to connected lights
                    if (connected[i]) {
                        Radio::Message msg(CMD_SET_RGB, colors[i]);
                        radio.send(i, msg);
                    }
                }
                sem_post(colors_sem);

                last_update = now;
            }
        }
    }

    while(1) {
        sleep(1);
    }

    return 0;
}

/**
 * Pings every endpoint and records the ones that respond.
 */
void pingAllLights(void) {
    // Generate the ping packet
    Radio::Message ping(CMD_PING);

    for (int i = 1; i < NUM_IDS; i++) {

        if (!radio.send(i, ping)) {
            // Packet wasn't delivered, so move on
            connected[i] = false;
            continue;
        }

        Radio::Message response;
        
        // Wait here until we get a response or timeout
        if (!radio.receive(response, PING_TIMEOUT)) {
            // Skip endpoints that timed out
            connected[i] = false;
            continue;
        }

        // Make sure the response checks out
        if (response.command != CMD_PING_RESPONSE ||
            response.data[0] != i) {

            connected[i] = false;
            continue;
        }

        printf("Found light %d\n", i);

        // Add the endpoint to the list
        connected[i] = true;
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
    uint8_t rgb[3] = {red, green, blue};
    Radio::Message msg(CMD_SET_RGB, rgb);
    return radio.send(endpoint, msg);
}

/**
 * Gets the temperature of the given light.
 * @param endpoint The endpoint to change
 * @param temp The place to store the temperature in millidegrees Celsius
 *
 * @return Whether the light acknowledged or not.
 */
bool getTemperature(uint8_t endpoint, int16_t * temp) {
    Radio::Message request(CMD_GET_TEMP);
    if (!radio.send(endpoint, request)) {
        // Sending failed
        return false;
    }

    Radio::Message response;
    if (!radio.receive(response, GET_TEMP_TIMEOUT)) {
        // Failed to get a response
        return false;
    }

    if (response.command != CMD_TEMP_RESPONSE) {
        // Invalid response
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
    Radio::Message request(CMD_GET_UPTIME);
    if (!radio.send(endpoint, request)) {
        // Sending failed
        return false;
    }

    Radio::Message response;
    if (!radio.receive(response, GET_UPTIME_TIMEOUT)) {
        // Failed to get a response
        return false;
    }

    if (response.command != CMD_UPTIME_RESPONSE) {
        // Invalid response
        return false;
    }

    *uptime = (response.data[0] << 8) | response.data[1];

    return true;
}

/**
 * Handles incoming WebSocket messages.
 *
 * @param client A handle representing the client
 * @param message The message received
 *
 * @todo Clean this up and handle multiple instructions per message
 */
void processMessage(connection_hdl client, const std::string message) {
    uint8_t id, r, g, b;

    if (!message.compare(0, 7, "setrgb ")) {
        // Set RGB
        if (sscanf(message.c_str(), "setrgb %hhu %hhu %hhu %hhu", &id, &r, &g, &b) == 4) {
            // Arguments are valid
            if (!connected[id]) {
                server.send(client, "Error: light not connected");
            }

            sem_wait(colors_sem);
            colors[id].r = r;
            colors[id].g = g;
            colors[id].b = b;
            sem_post(colors_sem);

            server.send(client, "OK");

        } else {
            // One or more arguments were invalid
            server.send(client, "Error: invalid arguments");
        }

    } else if (!message.compare("list")) {
        // List
        string list = "";
        for (int i = 1; i <= 0xff; i++) {
            if (connected[i]) {
                char num[5];
                sprintf(num, "%d,", i);
                list.append(num);
            }
        }
        server.send(client, list);

    } else if (!message.compare("discover")) {
        // Discover
        pingAllLights();
        server.send(client, "OK");

    } else if (!message.compare(0, 5, "temp ")) {
        // Set RGB
        if (sscanf(message.c_str(), "temp %hhu", &id) == 1) {
            // Arguments are valid
            int16_t temp;
            if (getTemperature(id, &temp)) {
                // The light acknowledged
                server.send(client, std::to_string(temp));
            } else {
                // The light did not acknowledge
                server.send(client, "Error: light not responding");
            }

        } else {
            // One or more arguments were invalid
            server.send(client, "Error: invalid arguments");
        }

    } else if (!message.compare(0, 7, "uptime ")) {
        // Set RGB
        if (sscanf(message.c_str(), "uptime %hhu", &id) == 1) {
            // Arguments are valid
            uint16_t uptime;
            if (getUptime(id, &uptime)) {
                // The light acknowledged
                server.send(client, std::to_string(uptime));
            } else {
                // The light did not acknowledge
                server.send(client, "Error: light not responding");
            }

        } else {
            // One or more arguments were invalid
            server.send(client, "Error: invalid arguments");
        }

    } else if (!message.compare(0, 4, "ping")) {
        // Echo the message - useful for measuring roundtrip latency
        server.send(client, message);

    } else {
        // Error
        server.send(client, "Error: unrecognized command");
    }
}

/**
 * Prints the welcome message for the serial console.
 */
void printWelcomeMessage(void) {
    printf("-------------------------------------\n");
    printf("ACM@UIUC SIGMusic Lights Base Station\n");
    printf("Version %x\n", VERSION);
    printf("This base station is endpoint %d\n", BASE_STATION_ID);
    printf("-------------------------------------\n");
}
