import bluetooth as bt
from light import Light
import websocket
import colorsys

# Stores each light
lights = {}

def find_lights():
    """ Finds nearby lights and adds them to the dictionary """
    nearby_devices = bt.discover_devices()

    for address in nearby_devices:
        name = bt.lookup_name(address)
        # Ignore non-lights
        if name.startswith("Light"):
            # Get the light number from the name
            num = int(name[-2:])
            # Check to see if we already found the light
            if not num in lights:
                # Create a light object
                print("Found light", num)
                lights[num] = Light(address, num)
            elif not lights[num].is_connected:
                # We found the light, but it wasn't able to connect. Try again
                print("Attempting to reconnect light", num)
                lights[num].connect_light()

    # Figure out how many lights are connected
    count = 0
    for light in lights:
        if lights[light].is_connected:
            count += 1
    
    if count == 0:
        print("No lights are connected!")
    return count

def cycle_hue():
    """Cycles the hue of each light"""
    while True:
        for light in lights:
            # Test all lights by cycling the hue
            for hue in [x/256 for x in range(0, 255)]:
                rgb = colorsys.hsv_to_rgb(hue, 1, 1)
                lights[light].send_rgb(int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2]))

def on_message(ws, message):
    print("Websocket:", message)

def on_error(ws, error):
    print("Websocket error:", error)
    print("Falling back to cycling hue")
    cycle_hue()

def on_close(ws):
    print("Websocket closed")

def on_open(ws):
    print("Websocket connected")

# Wait until at least one light has been discovered
# while not find_lights():
    # pass

# Connect to websocket from simulation
LIGHTS_PORT = 7445
ws_url = "ws://tonal-starfield.herokuapp.com:%s" % LIGHTS_PORT
print("Connecting to websocket (%s)..." % ws_url)
ws = websocket.WebSocketApp(ws_url,
                          on_open = on_open,
                          on_message = on_message,
                          on_error = on_error,
                          on_close = on_close)
ws.run_forever()
