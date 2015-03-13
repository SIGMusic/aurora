import bluetooth as bt
from light import Light
from comet import Comet
import colorsys
from time import sleep
import tornado.ioloop
import tornado.web
import tornado.websocket
import tornado.template
import json
from random import randint

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



class WSHandler(tornado.websocket.WebSocketHandler):
    def check_origin(self, origin):
        return True

    def open(self):
        print("Websocket connection opened")

    def on_message(self, message):
        print("Websocket message received:", message)

        json_dict = json.loads(message)
        rgb = json_dict["color"]
        hsl = json_dict["colorHSL"]
        pos = json_dict["position"]
        lifespan = json_dict["lifespan"]
        velocity = json_dict["velocity"]
        far = json_dict["far"]

        print("RGB:", (rgb["r"], rgb["g"], rgb["b"]))
        print("HSL:", (hsl["h"], hsl["s"], hsl["l"]))
        print("Position:", (pos["x"], pos["y"], pos["z"]))
        print("Velocity:", (velocity["x"], velocity["y"], velocity["z"]))
        print("Lifespan:", lifespan)
        print("Far:", far)
        # sleep(5)

        comet = Comet(pos, velocity, hsl, lifespan, far)
        comets.append(comet)

    def on_close(self):
        print("Websocket connection closed")

application = tornado.web.Application([
    (r'/', WSHandler),
])

def frame_update():
    # Update the lights based on each comet
    colors = []
    if len(comets) == 0:
        # twinkle twinkle
        for light in lights:
            cur = light.rgb
            w = 0.9
            val = w*cur[1] + (1-w)*(10-randint(1,20))
            lights[light].send_rgb(int(val), int(val), int(val))
        
        return

    # Get the colors from each comet
    for comet in comets:
        if comet.get_age() < comet.lifespan * 2:
            colors.append(comet.get_colors(lights))
            # sleep(1)
        else:
            print("Comet too old:", comet)
            comets.remove(comet)

    # print(colors)
    # print(len(colors))
    # sleep(1)
    # Get the average of the hues
    for light in lights:
        red = 0
        green = 0
        blue = 0
        for color in colors:
            red += color[light][0]
            green += color[light][1]
            blue += color[light][2]
        red = min(red*255/len(lights), 255)
        green = min(green*255/len(lights), 255)
        blue = min(blue*255/len(lights), 255)

        # Extremely "robust" error handling (read: weird bug avoidance)
        if red != red:
            red = 0
        if green != green:
            green = 0
        if blue != blue:
            blue = 0
        

        #twinkling
        
        

        print("Light", light, "RGB:", red, green, blue)
        lights[light].send_rgb(int(red), int(green), int(blue))


# Wait until at least one light has been discovered
while not find_lights():
    pass

# test_rgb()
# cycle_hue()

print("Starting websocket IO loop")
application.listen(7445)
timer = tornado.ioloop.PeriodicCallback(frame_update, 50)
timer.start()
tornado.ioloop.IOLoop.instance().start()
