"""Define various attributes and functions of the radio network."""

import signal
from RF24 import *

# TODO: share with scheduler, remove dummy data
colors = [(10, 10, 10), (20, 20, 20), (30, 30, 30), (40, 40, 40),
          (50, 50, 50), (60, 60, 60), (70, 70, 70), (80, 80, 80)]

# The hardcoded RF channel (for now)
RF_CHANNEL = 49

# The endpoint ID of the base station
BASE_STATION_ID = 0

# Radio GPIO pins
CE_PIN = RPI_V2_GPIO_P1_15  # Chip Enable pin
CSN_PIN = RPI_V2_GPIO_P1_24  # Chip Select pin

# Cap on the number of light updates per second
# TODO: read this from a config file
MAX_FPS = 120


def RF_ADDRESS(endpoint):
    """Return a 40-bit nRF address."""
    assert 0 <= endpoint < 256, "endpoint {0} is not between 0 and 255".format(endpoint)
    return (0x5349474D00 | endpoint)


class RadioNetwork:
    """Manage network operations on the radio.

    Each light is programmed with a unique 1-byte ID. Each has an
    nRF24L01+ radio module configured to listen on an address
    corresponding to its ID: the ASCII characters 'SIGM' followed by
    the ID. The Raspberry Pi base station also has an nRF24L01+ and
    follows the same address pattern with an ID of 0.

    The base station transmits an RGB value to each light, in order of
    ID, at a configurable frame rate. These messages are sent
    unreliably for minimal latency.

    """

    numLights = 8

    def __init__(self):
        """Initialize the network interface."""
        self._radio = RF24(CE_PIN, CSN_PIN)

        self._radio.begin()
        # 250kbps should be plenty. Lower speed decreases error rate.
        self._radio.setDataRate(RF24_250KBPS)
        # Higher power doesn't work on cheap eBay radios
        self._radio.setPALevel(RF24_PA_LOW)
        self._radio.setRetries(0, 0)
        self._radio.setAutoAck(False)
        self._radio.setCRCLength(RF24_CRC_16)
        self._radio.payloadSize = 3
        self._radio.setChannel(RF_CHANNEL)
        self._radio.openReadingPipe(1, RF_ADDRESS(BASE_STATION_ID))

        if __debug__:
            self._radio.printDetails()

    def start(self):
        """Start periodically transmitting frames."""
        # TODO: use sigwait loop to avoid locking mutex in signal handler
        signal.signal(signal.SIGALRM, self.transmitFrame)
        signal.setitimer(signal.ITIMER_REAL, 1.0/MAX_FPS, 1.0/MAX_FPS)

    def stop(self):
        """Stop periodically transmitting frames."""
        signal.setitimer(signal.ITIMER_REAL, 0)

    def transmitFrame(self, signum, frame):
        """Transmit the color of each light."""
        # TODO: mutex
        for i in range(self.numLights):
            msg = bytearray(colors[i])
            self._radio.openWritingPipe(RF_ADDRESS(i))
            self._radio.write(msg)
