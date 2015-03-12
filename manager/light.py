import bluetooth as bt

class Light:
    """A class to interact with Bluetooth SIGMusic lights"""

    def __init__(self, address, num):
        """ Constructor """
        self.num = num
        self.is_connected = False
        if bt.is_valid_address(address):
            self.endpoint = address
        else:
            raise ValueError("invalid Bluetooth address")
            return
        self.socket = self.connect_light()

    def __del__(self):
        """ Destructor """
        self.disconnect_light()

    def connect_light(self):
        """ Initiates the socket connection """
        # Should be using L2CAP because it is similar to UDP,
        # but it's only available on Linux. We want it to transmit RGB
        # data in real time, dropping packets if necessary. Order matters.
        # port = bt.get_available_port(bt.L2CAP) # Deprecated
        port = 1

        # Create the socket
        sock = bt.BluetoothSocket(bt.RFCOMM)
        
        # Attempt to connect
        try:
            sock.connect((self.endpoint, port))
        except:
            print("Could not connect to light", self.num)
            return
        self.is_connected = true

        return sock

    def disconnect_light(self):
        """ Closes the socket connection """
        # Close the socket gracefully
        self.socket.close()
        
        # Don't try to use the socket again
        self.socket = None

    def send_rgb(self, red, green, blue):
        """ Sends RGB values """
        if self.is_connected:
            checksum = (red + green + blue) % 256
            message = bytes([ord("S"), ord("I"), ord("G"), ord("M"), red, green, blue, checksum])
            self.socket.send(message)
