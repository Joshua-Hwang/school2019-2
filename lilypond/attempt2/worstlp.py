#!/usr/bin/env python3
import sys

import math

delta = 0.1

def print_usage():
    print("Usage:", __file__, "N", file=sys.stderr)
    print("\twhere N is the number of points to generate", file=sys.stderr)

# The reason I'm making a new class instead of grabbing from visualiser
# is because of the potential differences in how the grains will be represented
class Grain:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def __repr__(self):
        return "%f,%f,0.0,0.1,0,0" % (self.x, self.y)

# Takes the points and prints them to stdout for now
# [WIP] Get it to accept command line arguments to save to file
def main(N):
    ret = [Grain(i/100 * math.cos(math.pi*i/100), \
                 i/100 * math.sin(math.pi*i/100)) \
                 for i in range(int(N))]
    #ret = [Grain((1 + delta)**i, (1 + delta)**i) for i in range(int(N))]
    #    + [Grain(-i-1, i + 0.25) for i in range(int(N/4))] \
    #    + [Grain(-i-1, i - 0.25) for i in range(int(N/4))]

    return ret

if __name__ == "__main__":
    # Used for measuring performance
    import time

    if len(sys.argv) <= 1:
        print_usage()
        sys.exit(1)

    N = abs(int(sys.argv[1]))

    start = time.time()
    grains = main(N)
    end = time.time()
    generationTime = end - start

    print(__file__, "generation time:", generationTime, "secs", file=sys.stderr)
    print(__file__, "generated:", N, "points", file=sys.stderr)

    print("ZZZZZZZZZZ")
    for grain in grains:
        print(grain)
