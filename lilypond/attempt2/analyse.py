#!/usr/bin/env python3
import sys
import csv
import math

import collections
import argparse
import fileinput

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and analyses it")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

class Grain:
    def __init__(self, ID, x, y, t, v, r, i):
        self.id = ID
        self.x = x
        self.y = y
        self.t = t
        self.v = v
        self.r = r
        self.i = i
        self.lifeTime = self.r/self.v
        self.deathTime = self.lifeTime + t
        self.set = {self}

    def __hash__(self):
        return hash(self.id)

    # If they're in the same position we need to get rid of them
    def __eq__(self, other):
        if not isinstance(other, type(self)):
            return NotImplemented
        return self.id == other.id

    def __repr__(self):
        return "Grain(%d)" % self.id

# Consider producing graphs that show the distribution of radii pyplot.hist
def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith('#'))

    n = 0
    grains = []
    dimX, dimY, dimT = (math.inf, -math.inf), (math.inf, -math.inf), (0, 0)
    try: # parsing
        for row in reader:
            # Alphabet characters are used to halt parsing
            if row[0].isalpha():
                break
            x, y, t, v, r, i = row # see example.lp for these values
            # perform a very forgiving parse of the file
            x, y, t, v, r, i = float(x), float(y), \
                    abs(float(t)), abs(float(v)), \
                    abs(float(r)), abs(int(i))
            dimX = (min(dimX[0], x), max(dimX[1], x))
            dimY = (min(dimY[0], y), max(dimY[1], y))
            if v != 0:
                dimT = (0, math.ceil(max(dimT[1], t + r/v))) # this is determined by the birth and how long it takes to reach max radius
            grains.append(Grain(n, x, y, t, v, r, i))
            n += 1
    except ValueError as e:
        raise ValueError("Parsing error has occurred in %s at %s" % (fileinput.filename(), row[0]))

    # Go through each grain and union their sets (also tidy up the set you leave behind)
    for grain in (grain for grain in grains if grain.r > 0):
        otherGrain = grains[grain.i]
        grain.set.update(otherGrain.set)
        for g in grain.set:
            g.set = grain.set

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
    lifeTime = [grain.lifeTime for grain in grains]
    lifeTimeWithout = [t for t in lifeTime if t > 0]
    # Longest lifetime
    maxLife = max(lifeTime)
    maxGrains = [grain.id for grain in grains if grain.lifeTime == maxLife]
    print("Longest lifetime:", maxLife)
    print("From grains:", ", ".join(map(str, maxGrains)))
    # Average lifetime
    print("Average lifetime (with r=0):", sum(lifeTime)/len(lifeTime))
    print("Average lifetime (without r=0):", sum(lifeTimeWithout)/len(lifeTimeWithout))

    print("\n--=DEATHTIME=--")
    deathTime = [grain.deathTime for grain in grains]
    deathTimeWithout = [grain.deathTime for grain in grains if grain.r > 0]
    # Last surviving
    lastDeath = max(deathTime)
    maxGrains = [grain.id for grain in grains if grain.deathTime == lastDeath]
    print("Last death:", lastDeath)
    print("From grains:", ", ".join(map(str, maxGrains)))
    # Average deathtime
    print("Average deathtime (with r=0):", sum(deathTime)/len(deathTime))
    print("Average deathtime (without r=0):", sum(deathTimeWithout)/len(deathTimeWithout))

    # [WIP] Find the density. Currently not handling the boundary correctly.
    print("\n--=AREAS=--")
    coveredArea = sum((math.pi * r * r for r in radii))
    allArea = abs(dimX[1] - dimX[0]) * abs(dimY[1] - dimY[0])
    print("Covered Area:", coveredArea)
    print("All Area:", allArea)
    print("Density:", coveredArea/allArea);

    print("\n--=COMPONENTS=--")
    components = frozenset((frozenset(grain.set) for grain in grains if grain.r > 0))
    maxComponent = len(max(components, key=lambda component: len(component)))
    print("Number of components:", len(components))
    print("Largest connected component:", maxComponent)
    # [WIP] Find largest connected component
    # [WIP] Check for percolation from the top of the screen to the bottom
    # [WIP] Create graph of required tolerance to make it percolate

if __name__ == "__main__":
    main()
