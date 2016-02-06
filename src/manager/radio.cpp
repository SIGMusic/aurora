/**
 * SIGMusic Lights 2015
 * Radio class
 */

#include <iostream>
#include <arpa/inet.h>
#include "radio.h"


using std::cout;
using std::endl;


// Radio pins
#define CE_PIN                  RPI_GPIO_P1_15 // Radio chip enable pin
#define CSN_PIN                 RPI_GPIO_P1_24 // Radio chip select pin

// Wireless protocol
#define PING_TIMEOUT            10 // Time to wait for a ping response in milliseconds
#define GET_TEMP_TIMEOUT        10 // Time to wait for a temperature response in milliseconds
#define GET_UPTIME_TIMEOUT      10 // Time to wait for an uptime response in milliseconds


RF24 Radio::radio(CE_PIN, CSN_PIN);

color_t* Radio::colors;
sem_t* Radio::colors_sem;
uint32_t* Radio::connected;
sem_t* Radio::connected_sem;


Radio::Radio() {

    radio.begin();
    radio.setDataRate(RF24_250KBPS); // 250kbps should be plenty
    radio.setChannel(CHANNEL);
    radio.setPALevel(RF24_PA_MAX); // Range is important, not power consumption
    radio.setRetries(0, NUM_RETRIES);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPayloadSize(sizeof(packet_t));

    radio.openReadingPipe(1, RF_ADDRESS(BASE_STATION_ID));
    radio.startListening();

#ifdef DEBUG
    radio.printDetails();
#endif
}

void Radio::run(color_t* colors, sem_t* colors_sem,
    uint32_t* connected, sem_t* connected_sem) {
    
    Radio::colors = colors;
    Radio::colors_sem = colors_sem;
    Radio::connected = connected;
    Radio::connected_sem = connected_sem;

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
                    Message msg(CMD_SET_RGB, colors[i]);
                    send(i, msg);
                }
            }
            sem_post(colors_sem);

            last_update = now;
        }
    }
}

bool Radio::send(uint8_t endpoint, const Message & msg) {

    packet_t packet = {
        HEADER,
        msg.command,
        {msg.data[0], msg.data[1], msg.data[2]}
    };

    radio.stopListening();
    radio.openWritingPipe(RF_ADDRESS(endpoint));
    bool success = radio.write(&packet, sizeof(packet));
    radio.startListening();

    return success;
}

bool Radio::receive(Message & msg, unsigned int timeout) {

    unsigned int started_waiting_at = millis();
    bool wasTimeout = false;

    while (!radio.available() && !wasTimeout) {
        if ((millis() - started_waiting_at) > timeout) {
            wasTimeout = true;
        }
    }

    if (wasTimeout) {
        return false;
    }

    // Get the response
    packet_t response;
    radio.read(&response, sizeof(packet_t));

    // Make sure the response is valid
    if (response.header != HEADER) {
        return false;
    }

    // Copy the response into the client's buffer
    msg.command = response.command;
    std::copy(std::begin(response.data), std::end(response.data), std::begin(msg.data));

    return true;
}

/**
 * Pings every endpoint and records the ones that respond.
 */
void Radio::pingAllLights() {

    // Generate the ping packet
    Message ping(CMD_PING);

    sem_wait(connected_sem);

    for (int i = 1; i < NUM_IDS; i++) {

        if (!send(i, ping)) {
            // Packet wasn't delivered, so move on
            setLightConnected(i, false);
            continue;
        }

        Message response;
        
        // Wait here until we get a response or timeout
        if (!receive(response, PING_TIMEOUT)) {
            // Skip endpoints that timed out
            setLightConnected(i, false);
            continue;
        }

        // Make sure the response checks out
        if (response.command != CMD_PING_RESPONSE ||
            response.data[0] != i) {

            setLightConnected(i, false);
            continue;
        }

        printf("Found light %d\n", i);

        // Add the endpoint to the list
        setLightConnected(i, true);
    }

    sem_post(connected_sem);
}

void Radio::setLightConnected(uint8_t id, bool isConnected) {

    int index = id / sizeof(uint32_t);
    int bit = id % sizeof(uint32_t);

    uint32_t mask = 1 << bit;

    if (isConnected) {
        connected[index] |= mask;
    } else {
        connected[index] &= ~mask;
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
bool Radio::setRGB(uint8_t endpoint, uint8_t red, uint8_t green, uint8_t blue) {

    printf("Setting light %u to %u, %u, %u\n", endpoint, red, green, blue);
    uint8_t rgb[3] = {red, green, blue};
    Message msg(CMD_SET_RGB, rgb);
    return send(endpoint, msg);
}

/**
 * Gets the temperature of the given light.
 * @param endpoint The endpoint to change
 * @param temp The place to store the temperature in millidegrees Celsius
 *
 * @return Whether the light acknowledged or not.
 */
bool Radio::getTemperature(uint8_t endpoint, int16_t * temp) {

    Message request(CMD_GET_TEMP);
    if (!send(endpoint, request)) {
        // Sending failed
        return false;
    }

    Message response;
    if (!receive(response, GET_TEMP_TIMEOUT)) {
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
bool Radio::getUptime(uint8_t endpoint, uint16_t * uptime) {

    Message request(CMD_GET_UPTIME);
    if (!send(endpoint, request)) {
        // Sending failed
        return false;
    }

    Message response;
    if (!receive(response, GET_UPTIME_TIMEOUT)) {
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
