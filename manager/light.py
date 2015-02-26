import bluetooth as bt

class Light:
    """A class to interact with Bluetooth SIGMusic lights"""

    def __init__(self, address):
        """ Constructor """
        if is_valid_address(address):
            self.endpoint = address
        else:
            raise ValueError("invalid Bluetooth address")
            return
        self.socket = connect_light()

    def __del__(self):
        """ Destructor """
        disconnect_light()

    def connect_light():
        """ Initiates the socket connection """
        # Using L2CAP because it is similar to UDP. We want it to transmit RGB
        # data in real time, dropping packets if necessary. Order matters.
        port = bt.get_available_port(bt.L2CAP)

        # Create the socket
        sock = bt.BluetoothSocket(bt.L2CAP)
        
        # Attempt to connect
        sock.connect((self.endpoint, port))

        return sock

    def disconnect_light():
        """ Closes the socket connection """
        # Close the socket gracefully
        self.socket.close()
        
        # Don't try to use the socket again
        self.socket = None

    def send_rgb(red, green, blue):
        """ Sends RGB values """
        checksum = (red + green + blue) % 256
        self.socket.send("SIGM")
        self.socket.send(red)
        self.socket.send(green)
        self.socket.send(blue)
        self.socket.send(checksum)
