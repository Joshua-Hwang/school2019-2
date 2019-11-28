#!/usr/bin/env python3
import sys
import csv
import math

import argparse
import fileinput

from matplotlib import pyplot as plt
from matplotlib import animation

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and generates a visual")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

class Circle:
    def __init__(self, ID, x, y, t, v, r, i):
        self.id = ID
        self.xy = (x, y)
        self.t = t
        self.v = v
        self.r = r
        self.i = i
        self.shape = plt.Circle(self.xy, 0)

    def __repr__(self):
        return "Circle:{id: %d, x: %f, y: %f, t: %f, v: %f, r: %f, i: %d}" \
                % (self.id, *self.xy, self.t, self.v, self.r, self.i)

def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith("#"))

    circles = []
    dimX, dimY, dimT = (math.inf, -math.inf), (math.inf, -math.inf), (0, 0)
    n = 0 # this is the index of the next circle
    try: # parsing
        for row in reader:
            x, y, t, v, r, i = row # see example.lp for these values
            # perform a very forgiving parse of the file
            x, y, t, v, r, i = float(x), float(y), abs(float(t)), abs(float(v)), abs(float(r)), abs(int(i))
            dimX = (min(dimX[0], x), max(dimX[1], x))
            dimY = (min(dimY[0], y), max(dimY[1], y))
            dimT = (0, math.ceil((max(dimT[1], t + r/v)))) # this is determined by the birth and how long it takes to reach max radius
            circles.append(Circle(n, x, y, t, v, r, i))
            n += 1
    except ValueError as e:
        raise ValueError("Parsing error has occurred in %s at %s" % (fileinput.filename(), row[0]))

    # check that all i < n
    for circle in (circle for circle in circles if circle.i >= n):
        raise IndexError("circle %d collides with circle %d which does not exist" % (circle.id, circle.i))

    visualise(circles, dimX, dimY, dimT)


# displays the list of circles as a matplotlib figure
# [WIP] maybe develop the collision graph
def visualise(circles, dimX, dimY, dimT):
    fig = plt.figure()
    ax = plt.axes(xlim=dimX, ylim=dimY)
    artistShapes = [circle.shape for circle in circles] # needed for init() and animate()

    def init():
        for circle in circles:
            ax.add_patch(circle.shape)
        return artistShapes

    def animate(i):
        for circle in circles:
            # ignore circle if radius <= 0 (never born) or birth time hasn't happened yet
            r = circle.v * (i - circle.t)
            if r < circle.r:
                circle.shape.set_radius(max(r,0))
            else:
                circle.shape.set_radius(circle.r)
        return artistShapes

    anim = animation.FuncAnimation(fig, animate, init_func=init,
            frames=dimT[1] + 100, interval=1, blit=True)

    plt.axis('scaled')
    plt.show()


if __name__ == "__main__":
    main()
