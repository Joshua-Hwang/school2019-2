#!/usr/bin/env python3
import sys
import csv
import math

import collections
import argparse
import fileinput

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and approximately solves it")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

deltaT = 1 # we take small steps

class Grain:
    """
    Simple class used to store information regarding
    each grain.
    """
    def __init__(self, ID, x, y, t, v):
        self.id = ID
        self.x = x
        self.y = y
        self.t = t
        self.v = v
        self.r = 0
        self.i = 0

    def __hash__(self):
        return hash(self.id)

    # If they're in the same position we need to get rid of them
    def __eq__(self, other):
        if not isinstance(other, type(self)):
            return NotImplemented
        return self.id == other.id

    def __repr__(self):
        return "%f,%f,%f,%f,%f,%d" % \
                (self.x, self.y, self.t, self.v, self.r, self.i)

class GrainPair:
    """
    Simple class used to store information regarding
    each grain pair. Does some caching as well.
    """
    def __init__(self, left, right):
        self.left = left
        self.right = right
        self.lastUpdate = -1
        self.lastValue = -1
        self.dist = -1

    def __hash__(self):
        return hash((self.left, self.right))

    # If they're in the same position we need to get rid of them
    def __eq__(self, other):
        if not isinstance(other, type(self)):
            return NotImplemented
        return self.left == other.left and self.right == other.right

    def calc_gap(self, t):
        # first check if pair has already been evaluated
        if abs(self.lastUpdate - t) >= deltaT:
            if self.dist < 0:
                distX = self.left.x - self.right.x
                distY = self.left.y - self.right.y
                self.dist = math.sqrt(distX**2 + distY**2)
            # update grainPair
            self.lastUpdate = t
            self.lastValue = self.dist - self.left.r - self.right.r
        return self.lastValue

    def __repr__(self):
        # print smallest first
        return "(" + str(self.left.id) + ", " + str(self.right.id) + ")" \
                if self.left.id < self.right.id else \
                "(" + str(self.right.id) + ", " + str(self.left.id) + ")"

# Consider producing graphs that show the distribution of radii pyplot.hist
def main():
    """ This is the main function """
    reader = csv.reader(row for row in fileinput.input() if not row.startswith("#"))

    n = 0
    grains = []
    unbornGrains = [] # order the list by born times
    growingGrains = set()
    grownGrains = set() # only add grains that have actually grown and didn't die
    dimX, dimY = (math.inf, -math.inf), (math.inf, -math.inf)
    try: # parsing
        for row in reader:
            x, y, t, v, r, i = row # see example.lp for these values
            # perform a very forgiving parse of the file
            x, y, t, v, r, i = float(x), float(y), \
                    abs(float(t)), abs(float(v)), \
                    abs(float(r)), abs(int(i))
            dimX = (min(dimX[0], x), max(dimX[1], x))
            dimY = (min(dimY[0], y), max(dimY[1], y))

            grain = Grain(n, x, y, t, v)
            grains.append(grain)
            unbornGrains.append(grain)
            n += 1
    except ValueError as e:
        raise ValueError("Parsing error has occurred in %s at %s" % (fileinput.filename(), row[0]))
    # First generate pairs with growingGrains, grownGrains and grain
    grainPairs = set()
    # Check immediately if any of the generated pairs are wrong (negative distance)
    # Convert the unbornGrains list into a sorted deque
    unbornGrains = collections.deque(sorted(unbornGrains, key=lambda g: g.t))
    # Once all grains are obtained begin stepping through time
    t = 0
    steps = 0
    while unbornGrains or growingGrains:
        # grow the growing grains
        for grain in growingGrains:
            grain.r = grain.v * (t - grain.t)
        # Once the grains time has come bring them into the growingGrains
        # generate pairs involving these grains
        while unbornGrains and unbornGrains[0].t < t:
            grain = unbornGrains.popleft()
            # Add them to pairedGrains
            pairs = [GrainPair(grain, growingGrain) \
                    for growingGrain in growingGrains] + \
                    [GrainPair(grain, grownGrain) \
                    for grownGrain in grownGrains]
            # ensure the current grain isn't suffocated
            if any((pair.calc_gap(t) < 0 for pair in pairs)):
                # Don't include it
                continue
            grainPairs.update(pairs)
            growingGrains.add(grain)
        # Evaluate all pairs that have negative calc_gap
        evaluate = [grainPair for grainPair in grainPairs \
                if grainPair.calc_gap(t) < 0]
        for pair in evaluate:
            # remove pair from grainPairs
            grainPairs.discard(pair)
            # evaluate both parts of the pair if growing
            if pair.left not in grownGrains:
                growingGrains.discard(pair.left)
                # Set i
                pair.left.i = pair.right.id
                # move growingGrains to grownGrains
                grownGrains.add(pair.left)
            if pair.right.r not in grownGrains:
                growingGrains.discard(pair.right)
                pair.right.i = pair.left.id
                grownGrains.add(pair.right)
        steps += 1
        t = steps * deltaT

    for grain in grains:
        print(grain)

if __name__ == "__main__":
    #import cProfile
    #cProfile.run("main()")
    # Used for measuring performance
    import time
    import sys

    start = time.time()
    main()
    end = time.time()
    solvingTime = end - start

    print(__file__, "solve time:", solvingTime, file=sys.stderr)
