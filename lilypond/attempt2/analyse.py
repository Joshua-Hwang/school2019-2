#!/usr/bin/env python3
import sys
import csv
import math

import collections
import argparse
import fileinput

import lp

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and analyses it")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

# Consider producing graphs that show the distribution of radii pyplot.hist
def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith('#'))

    parsed = lp.parse_all(reader)
    grains = parsed["grains"]

    n = len(grains)
    dimX, dimY, dimT = (math.inf, -math.inf), (math.inf, -math.inf), (0, 0)
    calcDelta = False
    if "dims" in parsed:
        dimX = (parsed["dims"]["min_0"], parsed["dims"]["max_0"])
        dimY = (parsed["dims"]["min_1"], parsed["dims"]["max_1"])
        if "collisions" in parsed:
            calcDelta = True

    extensions = parsed.get("collisions")

    # determine all grains that have been touched this involves creating a special
    # object the the top
    if calcDelta:
        topBorder = set()
        bottomBorder = set()
        for grain in (grain for grain in grains if grain.r > 0):
            # topBorder or bottomBorder if the location + radius
            if grain.y + grain.r >= dimY[1]:
                topBorder.update(grain.set)
                for g in grain.set:
                    g.set = topBorder
            if grain.y - grain.r <= dimY[0]:
                bottomBorder.update(grain.set)
                for g in grain.set:
                    g.set = bottomBorder
    
    # Go through each grain and union their sets (also tidy up the set you leave behind)
    for grain in (grain for grain in grains if grain.r > 0 and grain.i >= 0):
        otherGrain = grains[grain.i]
        grain.set.update(otherGrain.set)
        for g in grain.set:
            g.set = grain.set

    print("XXXXXXXX")
    print("Number of grains:", n)

    print("\n--=RADIUS=--")
    radii = [grain.r for grain in grains]
    radiiWithout = [r for r in radii if r > 0]
    # Find the largest radius
    maxRadius = max(radii)
    maxGrains = [grain.id for grain in grains if grain.r == maxRadius]
    print("Largest radius:", maxRadius)
    print("From grains:", ", ".join(map(str, maxGrains)))
    # Find the average radius with and without suffocated
    print("Average radius (with r=0):", sum(radii)/len(radii))
    print("Average radius (without r=0):", sum(radiiWithout)/len(radiiWithout))

    print("\n--=LIFETIME=--")
    life_time = [grain.life_time for grain in grains]
    life_timeWithout = [t for t in life_time if t > 0]
    # Longest lifetime
    maxLife = max(life_time)
    maxGrains = [grain.id for grain in grains if grain.life_time == maxLife]
    print("Longest lifetime:", maxLife)
    print("From grains:", ", ".join(map(str, maxGrains)))
    # Average lifetime
    print("Average lifetime (with r=0):", sum(life_time)/len(life_time))
    print("Average lifetime (without r=0):", sum(life_timeWithout)/len(life_timeWithout))

    print("\n--=DEATHTIME=--")
    death_time = [grain.death_time for grain in grains]
    death_timeWithout = [grain.death_time for grain in grains if grain.r > 0]
    # Last surviving
    lastDeath = max(death_time)
    maxGrains = [grain.id for grain in grains if grain.death_time == lastDeath]
    print("Last death:", lastDeath)
    print("From grains:", ", ".join(map(str, maxGrains)))
    # Average deathtime
    print("Average deathtime (with r=0):", sum(death_time)/len(death_time))
    print("Average deathtime (without r=0):", sum(death_timeWithout)/len(death_timeWithout))

    # [WIP] Find the density. Currently not handling the boundary correctly.
    if "dims" in parsed:
        print("\n--=AREAS=--")
        coveredArea = sum((math.pi * r * r for r in radii))
        allArea = abs(dimX[1] - dimX[0]) * abs(dimY[1] - dimY[0])
        print("Covered Area:", coveredArea)
        print("All Area:", allArea)
        print("Density:", coveredArea/allArea)

    print("\n--=COMPONENTS=--")
    components = frozenset((frozenset(grain.set) for grain in grains if grain.r > 0))
    maxComponent = len(max(components, key=len))
    print("Number of components:", len(components))
    print("Largest connected component:", maxComponent)
    # [WIP] Find largest connected component
    # [WIP] Check for percolation from the top of the screen to the bottom
    # [WIP] Create graph of required tolerance to make it percolate

    delta = 0
    if calcDelta:
        while not topBorder.intersection(bottomBorder) and extensions:
            # Determine if the topBorder is the bottomBorder else continue
            collision = extensions.pop()
            delta = max(delta, collision.dist)
            collision.grain_i.set.update(collision.grain_j.set)
            for g in collision.grain_i.set:
                g.set = collision.grain_i.set
            if topBorder.intersection(collision.grain_i.set):
                for g in topBorder:
                    g.set = collision.grain_i.set
                topBorder = collision.grain_i.set
            if bottomBorder.intersection(collision.grain_i.set):
                for g in bottomBorder:
                    g.set = collision.grain_i.set
                bottomBorder = collision.grain_i.set

        print("Percolation occurs at delta:", delta)

if __name__ == "__main__":
    main()
