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
            # Get the light number from the Bluetooth name
            num = int(name[-2:])
            # Check to see if we already found the light
            if not num in lights:
                # Create a light object
                print("Found light", num)
                lights[num] = Light(address, num)

    if lights:
        return len(lights)
    else:
        print("Couldn't find any lights!")
        return 0


# Wait until at least one light has been discovered
while not find_lights():
    pass

while True:
    for light in lights:
        # Test all lights by cycling the hue
        for hue in [x/256 for x in range(0, 255)]:
            rgb = colorsys.hsv_to_rgb(hue, 1, 1)
            lights[light].send_rgb(int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2]))
