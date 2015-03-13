import bluetooth as bt
from light import Light
from comet import Comet
# from SimpleWebSocketServer import WebSocket, SimpleWebSocketServer
import json
import colorsys
from time import sleep


# Stores each light
lights = {}

# Stores each comet
comets = []

def find_lights():
    """ Finds nearby lights and adds them to the dictionary """
    print("Discovering devices...")
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
        count = 0
        for light in lights:
            if lights[light].is_connected:
                count += 1
                # Test all lights by cycling the hue
                for hue in [x/256 for x in range(0, 255)]:
                    rgb = colorsys.hsv_to_rgb(hue, 1, 1)
                    lights[light].send_rgb(int(255*rgb[0]), int(255*rgb[1]), int(255*rgb[2]))
        if count == 0:
            print("No connected lights. Quitting.")
            quit()

def test_rgb():
    """ Tests each channel of each light sequentially """
    while True:
        for light in lights:
            if lights[light].is_connected:
                print("Testing light", light)
                print("Red")
                lights[light].send_rgb(255, 0, 0)
                sleep(1)
                print("Green")
                lights[light].send_rgb(0, 255, 0)
                sleep(1)
                print("Blue")
                lights[light].send_rgb(0, 0, 255)
                sleep(1)
                lights[light].send_rgb(0, 0, 0)


# class TonalLightfieldSocket(WebSocket):

#     def handleMessage(self):
#         # A comet was created
#         if self.data is None:
#             self.data = ''

#         print("Websocket:", str(self.data))
#         json_dict = json.loads(str(self.data))
#         rgb = json_dict.color
#         hsl = json_dict.colorHSL
#         pos = json_dict.position
#         lifespan = json_dict.lifespan

#         print("RGB:", (rgb["r"], rgb["g"], rgb["b"]))
#         print("HSL:", (hsl["h"], hsl["s"], hsl["l"]))
#         print("Position:", (pos["x"], pos["y"], pos["z"]))
#         print("Lifespan:", lifespan)

#         comet = Comet(pos, hsl, lifespan)
#         comets.append(comet)

#     def handleConnected(self):
#         print("Websocket connected:", self.address)

#     def handleClose(self):
#         print("Websocket closed:", self.address)

# LIGHTS_PORT = 7445
# server = SimpleWebSocketServer('', LIGHTS_PORT, TonalLightfieldSocket)
# server.serveforever()

# Wait until at least one light has been discovered
while not find_lights():
    pass

test_rgb()

# Update the lights based on each comet
while True:
    # Get the colors from each comet
    colors = []
    for comet in comets:
        if comets[comet].get_age() > comets[comet].lifespan:
            comets.remove(comet)
        else:
            colors.append(comets[comet].get_colors())

    # Get the average of the hues
    mixed_colors = {}
    for light in lights:
        rgb = {}
        rgb["r"] = max(sum(colors[:][light]["r"]), 255)
        rgb["g"] = max(sum(colors[:][light]["g"]), 255)
        rgb["b"] = max(sum(colors[:][light]["b"]), 255)

        mixed_colors[light] = rgb
