import colorsys
import time
from math import sqrt

class Comet:
    """A class to represent a Comet and calculate color components for lights"""

    def __init__(self, pos, hsl, lifespan):
        """ Constructor """
        self.initial_position = (pos["x"], pos["y"], pos["z"])
        self.hsl = hsl
        self.lifespan = lifespan # time for the comet to live in seconds
        self.created = time.time() # Get the timestamp of creation in seconds
        # The comet will move from pos.z to 0 in the z direction over lifespan seconds
        self.velocity = pos["z"]/lifespan

    def get_age(self):
        """ Returns the age of the comet in seconds """
        return time.time() - self.created

    def get_position(self):
        """ Returns the current position of the comet """
        age = self.get_age()
        pos = self.initial_position
        new_pos = (pos[0], pos[1], pos[2] - age*self.velocity)
        return new_pos

    def get_colors(self, lights):
        """ Returns a dictionary containing the HSL color the comet contributes
            to each light at the current time """
        pos = self.get_position()

        # Normalize the position to x = [-1, 1] and z = [0, 3]
        normalized_pos = (pos[0]/1000, pos[1]/1000, pos[2]/1000)

        color = {}
        for light in lights:
            # Calculate Euclidean distance to each light
            dpos = (normalized_pos[0] - lights[light].pos[0],
                    normalized_pos[1] - lights[light].pos[1],
                    normalized_pos[2] - lights[light].pos[2])
            distance = sqrt(dpos[0]**2 + dpos[1]**2 + dpos[2]**2)

            # Calculate the color contribution from each light
            hsl = self.hsl
            hsl["l"] /= distance**2 # Intensity falls as distance^2
            color[light] = 255*colorsys.hsv_to_rgb(hsl["h"], hsl["s"], hsl["l"])

        return color
