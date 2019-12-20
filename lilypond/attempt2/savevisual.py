#!/usr/bin/env python3
import sys
import csv
import math

import argparse
import fileinput

import lp

from matplotlib import pyplot as plt
from matplotlib import animation

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and generates a visual")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

showLines = True

def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith('#'))

    circles = lps.parse_lps(reader)
    dimX, dimY, dimT = (math.inf, -math.inf), (math.inf, -math.inf), (0, 0)

    # check that all i < n
    for circle in (circle for circle in circles if circle.i >= len(circles)):
        raise IndexError("circle %d collides with circle %d which does not exist" % (circle.id, circle.i))

    for circle in circles:
        circle.shape = plt.Circle((circle.x,circle.y), 0, color='cornflowerblue')
        dimX = (min(dimX[0], circle.x), max(dimX[1], circle.x))
        dimY = (min(dimY[0], circle.y), max(dimY[1], circle.y))
        if circle.v != 0:
            dimT = (0, math.ceil(max(dimT[1], circle.t + circle.r/circle.v))) # this is determined by the birth and how long it takes to reach max radius

    visualise(circles, dimX, dimY, dimT)
    print(__file__, "num frames:", dimT[1])


# displays the list of circles as a matplotlib figure
# [WIP] maybe develop the collision graph
def visualise(circles, dimX, dimY, dimT):
    #fig, ax = plt.subplots()
    fig = plt.figure()
    ax = plt.axes(xlim=dimX, ylim=dimY)

    # append a line, = ax.plot(x, y, color='k') to artistShapes
    artistShapes = [circle.shape for circle in circles] # needed for init() and animate()

    # Generate lines, circle generation happens in Circle.__init__()
    if showLines:
        for circle in circles:
            otherCircle = circles[circle.i]
            x2x = (circle.x, otherCircle.x)
            y2y = (circle.y, otherCircle.y)
            circle.line = ax.plot(x2x, y2y, color='lightcoral', linewidth=1, visible=False)[0]

        artistShapes += [circle.line for circle in circles]

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
                if showLines:
                    circle.line.set_visible(False)
            else:
                # show line between each circle
                circle.shape.set_radius(circle.r)
                if showLines and circle.r > 0:
                    circle.line.set_visible(True)
        return artistShapes

    anim = animation.FuncAnimation(fig, animate, init_func=init,
            frames=dimT[1] + 100, interval=1, blit=True)

    plt.axis('scaled')
    anim.save('test.mp4', fps=15, dpi=240)

if __name__ == "__main__":
    main()
