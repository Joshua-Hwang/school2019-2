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

showLines = False

class Circle:
    def __init__(self, ID, x, y, t, v, r, i):
        self.id = ID
        self.x = x
        self.y = y
        self.t = t
        self.v = v
        self.r = r
        self.i = i
        # line generation happens in visualise()
        self.shape = plt.Circle((x,y), r, color='cornflowerblue')

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
            if v != 0:
                dimT = (0, math.ceil(max(dimT[1], t + r/v))) # this is determined by the birth and how long it takes to reach max radius
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
    #fig, ax = plt.subplots()
    fig = plt.figure()
    ax = plt.axes(xlim=dimX, ylim=dimY)

    # Generate lines, circle generation happens in Circle.__init__()
    if showLines:
        for circle in circles:
            otherCircle = circles[circle.i]
            x2x = (circle.x, otherCircle.x)
            y2y = (circle.y, otherCircle.y)
            circle.line = ax.plot(x2x, y2y, color='lightcoral', linewidth=1, visible=False)[0]

    # append a line, = ax.plot(x, y, color='k') to artistShapes
    artistShapes = [circle.shape for circle in circles] \
            + [circle.line for circle in circles] if showLines else [] # needed for init() and animate()

    for circle in circles:
        ax.add_patch(circle.shape)

    if showLines:
        for circle in circles:
            # show line between each circle
            if circle.r > 0 and showLines:
                circle.line.set_visible(True)

    plt.axis('scaled')
    plt.show()


if __name__ == "__main__":
    main()
