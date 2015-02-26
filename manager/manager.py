import bluetooth as bt
from light import Light

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
                lights[num] = Light(address)
                # print "Found light", num

    if lights:
        print "Found lights: ", lights.keys()
        return len(lights)
    else:
        print "Couldn't find any lights!"
        return 0


# Wait until at least one light has been discovered
while not find_lights():
    pass

for light in endpoints:
    # Test all lights by setting them to red
    lights[light].send_rgb(255, 0, 0)
