import colorsys
import time
from math import sqrt, log

class Comet:
    """A class to represent a Comet and calculate color components for lights"""

    def __init__(self, pos, velocity, hsl, lifespan, far):
        """ Constructor """
        self.initial_position = (pos["x"], pos["y"], pos["z"])
        self.velocity = (velocity["x"], velocity["y"], velocity["z"])
        self.hsl = hsl
        self.lifespan = lifespan # time for the comet to live in seconds
        self.created = time.time() # Get the timestamp of creation in seconds

    def get_age(self):
        """ Returns the age of the comet in seconds """
        return time.time() - self.created

    def get_position(self):
        """ Returns the current position of the comet """
        age = self.get_age()
        new_pos = (self.initial_position[0],# + age*self.velocity[0]/10,
                   self.initial_position[1],# + age*self.velocity[1]/10,
                   self.initial_position[2] + age*-200)#self.velocity[2]/10)
        return new_pos

    def get_colors(self, lights):
        """ Returns a dictionary containing the HSL color the comet contributes
            to each light at the current time """
        # age = self.get_age()
        # color = {}
        # for light in lights:
        #     hsl = self.hsl.copy()
        #     hsl["l"] = hsl["l"] /age
        #     color[light] = colorsys.hsv_to_rgb(hsl["h"], hsl["s"], hsl["l"])
        # return color


        pos = self.get_position()
        print(pos)
        color = {}
        for light in lights:
            # Calculate Euclidean distance to each light
            dpos = (pos[0] - lights[light].pos[0],
                    pos[1] - lights[light].pos[1],
                    pos[2] - lights[light].pos[2])
            distance = sqrt(dpos[0]**2 + dpos[1]**2 + dpos[2]**2)
            print("Distance:", distance)

            # Calculate the color contribution from each light
            hsl = self.hsl.copy()
            hsl["l"] = hsl["l"] * 1000000 / distance**2 # Intensity falls as distance^2
            color[light] = colorsys.hsv_to_rgb(hsl["h"], hsl["s"], hsl["l"])

        return color
