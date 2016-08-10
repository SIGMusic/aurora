"""Define various attributes and functions of the radio network."""

import signal
import threading
import logging
from datetime import datetime

try:
    from RF24 import *
    RF = True
except ImportError:
    RF = False


colors = []
colors_lock = threading.Lock()
colors_lock_lock = threading.Lock()

# The hardcoded RF channel (for now)
RF_CHANNEL = 49

# The endpoint ID of the base station
BASE_STATION_ID = 0

if RF:
    # Radio GPIO pins
    CE_PIN = RPI_V2_GPIO_P1_15  # Chip Enable pin
    CSN_PIN = RPI_V2_GPIO_P1_24  # Chip Select pin

# Cap on the number of light updates per second
# TODO: read this from a config file
MAX_FPS = 1


class RadioNetwork:

    """Manage network operations on the radio.

    Each light is programmed with a unique 1-byte ID. Each has an
    nRF24L01+ radio module configured to listen on an address
    corresponding to its ID: the ASCII characters 'SIGM' followed by
    the one-byte endpoint ID. The Raspberry Pi base station also has an
    nRF24L01+ and follows the same address pattern with an ID of 0.

    The base station transmits an RGB value to each light, in order of
    ID, at a configurable frame rate. These messages are sent
    without acknowledgement for minimal latency.

    """

    num_lights = 8

    def __init__(self):
        """Initialize the network interface."""
        self._logger = logging.getLogger(__name__)
        if RF:
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
            self._radio.openReadingPipe(1, self.rf_address(BASE_STATION_ID))

            if __debug__:
                self._radio.printDetails()

    def start(self):
        """Start periodically transmitting frames."""
        # TODO: use sigwait loop to avoid locking mutex in signal handler
        signal.signal(signal.SIGALRM, self.transmit_frame)
        signal.setitimer(signal.ITIMER_REAL, 1.0/MAX_FPS, 1.0/MAX_FPS)

    def stop(self):
        """Stop periodically transmitting frames."""
        signal.setitimer(signal.ITIMER_REAL, 0)

    def transmit_frame(self, signum, frame):
        """Transmit the color of each light."""
        colors_lock_lock.acquire()
        colors_lock.acquire()
        for i in range(self.num_lights):
            # For now, just print values instead of transmitting
            try:
                rgb = colors[i]
            except IndexError:
                rgb = (0, 0, 0)
            self._logger.debug(rgb)
            if RF:
                msg = bytearray(rgb)
                self._radio.openWritingPipe(rf_address(i))
                self._radio.write(msg)
        colors_lock.release()
        colors_lock_lock.release()

    def rf_address(self, endpoint):
        """Return a 40-bit nRF address."""
        if not 0 <= endpoint < 256:
            raise ValueError
        address_header = "SIGM"
        return int("0x"
                   + "".join(["%0.2x" % ord(i) for i in address_header])
                   + "%0.2x" % endpoint,
                   16)
