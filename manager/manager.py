from RF24 import *
import time

# Radio pins
CE_PIN  = RPI_BPLUS_GPIO_J8_22 # Radio chip enable pin
CSN_PIN = RPI_BPLUS_GPIO_J8_24 # Radio chip select pin

# Wireless protocol
def HASH(a):
    # Simple 1:1 single byte hash to minimize repeating bit patterns in address
    return a ^ 73

def RF_ADDRESS(endpoint):
    # Generates a 40-bit nRF address
    return 0x5349474D00 | HASH(endpoint & 0xFF)

HEADER                = 7446 # Must prefix every message
BASE_STATION_ID       = 0x00 # The endpoint ID of the base station
MULTICAST_ID          = 0xFF # The endpoint ID that all lights listen to

ENDPOINT_PIPE         = 1    # The pipe for messages for this endpoint

CHANNEL               = 80   # Channel center frequency = 2.4005 GHz + (Channel# * 1 MHz)
NUM_RETRIES           = 5    # Number of auto retries before giving up
PING_TIMEOUT          = 0.01   # Time to wait for a ping response in seconds

CMD_SET_RGB           = 0x10
CMD_PING              = 0x20
CMD_PING_RESPONSE     = 0x21

PACKET_SIZE           = 6

def parse_packet(raw):
    return {
        "header": (raw[1] << 8) | raw[0],
        "command": raw[2],
        "data": [raw[3], raw[4], raw[5]]
    }

def build_packet(command = 0, data = (0, 0, 0)):
        return bytearray([
            HEADER & 0x00ff, # Arduino is little-endian, so adjust accordingly
            (HEADER & 0xff00) >> 8,
            command,
            data[0],
            data[1],
            data[2]
        ])

# Software information
VERSION               = 0 # The software version number


radio = RF24(CE_PIN, CSN_PIN, BCM2835_SPI_SPEED_8MHZ)
lights = []

endpointID = BASE_STATION_ID

'''
Prints the welcome message.
'''
def printWelcomeMessage():
    print "-------------------------------------"
    print "SIGMusic@UIUC Lights Base Station"
    print "Version ", VERSION
    print "This base station is endpoint 0x00"
    print "-------------------------------------"

'''
Pings every endpoint and records the ones that respond.
'''
def ping_all_lights():
    # Generate the ping packet
    ping = build_packet(CMD_PING)

    for i in range(1, 0xff):
        radio.stopListening()
        radio.openWritingPipe(RF_ADDRESS(i))
        success = radio.write(ping)
        radio.startListening()

        # If the packet wasn't delivered after several attempts, move on
        if (not success):
            continue
        
        # Wait here until we get a response or timeout
        started_waiting_at = time.time()
        timeout = False
        while (not radio.available()) and (not timeout):
            if ((time.time() - started_waiting_at) > PING_TIMEOUT):
                timeout = True

        # Skip endpoints that timed out
        if (timeout):
            continue

        # Get the response
        response = build_packet()
        radio.read(response, len(response))
        parsed = parse_packet(response)

        # Make sure the response checks out
        if (parsed["header"] != HEADER or
            parsed["command"] != CMD_PING_RESPONSE or
            parsed["data"][0] != i):
            continue

        print "Found light", format(i, '#04x')

        # Add the endpoint to the list
        lights.append(i)




# Initialize the radio
radio.begin()
radio.setDataRate(RF24_250KBPS); # 250kbps should be plenty
radio.setChannel(CHANNEL)
radio.setPALevel(RF24_PA_MAX); # Range is important, not power consumption
radio.setRetries(0, NUM_RETRIES)
radio.setCRCLength(RF24_CRC_16)
radio.payload_size = PACKET_SIZE

radio.openReadingPipe(ENDPOINT_PIPE, RF_ADDRESS(endpointID))
radio.startListening()

printWelcomeMessage()

radio.printDetails()


print "Scanning for lights..."
ping_all_lights()
print "Done scanning."


# Broadcast red to all lights as a test
packet = build_packet(CMD_SET_RGB, [255, 0, 0])
radio.stopListening()
radio.openWritingPipe(RF_ADDRESS(MULTICAST_ID))
radio.write(packet)
radio.startListening()

