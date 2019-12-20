#!/usr/bin/env python3
import sys

import numpy as np
from scipy.integrate import nquad # we use this to integrate for higher dimensions this must be changed
from multiprocessing import Pool
from functools import partial

homogeneous = True
useThreads = False

dimX = (0, 1)
dimY = (0, 1)
dimT = (0, 100)
dimV = (0.05, 0.05)

# numerical integration is slow. Precompute for speed. None for numeric.
maxLam = float(sys.argv[1]) if len(sys.argv) > 1 else 200
# lam is short for the Poisson parameter lambda (not the functional thing)
def lamDensity(x, y, t, v):
    if homogeneous:
        return maxLam
    return 2000
    return np.exp(-1/2*(x**2 + y**2)) * 10
    return 1/(t+1) * np.exp(-1/2*(x**2 + y**2)) * 10
    return 1/(t+1) * np.sin(x)**2 * 10

# The reason I'm making a new class instead of grabbing from visualiser
# is because of the potential differences in how the grains will be represented
class Grain:
    def __init__(self, x, y, t, v):
        self.x = x
        self.y = y
        self.t = t
        self.v = v

    def __repr__(self):
        return "%f,%f,%f,%f,0,0" % (self.x, self.y, self.t, self.v)

# Takes the points and prints them to stdout for now
# [WIP] Get it to accept command line arguments to save to file
def main(dimX, dimY, dimT, dimV, lamDensity, maxLam):
    # single grain simulation is boring
    volume = (np.diff(dimX) * np.diff(dimY))[0]
    if np.diff(dimT)[0] > 0.00001:
        volume *= np.diff(dimT)[0]
    if np.diff(dimV)[0] > 0.00001:
        volume *= np.diff(dimV)[0]

    maxN = np.random.poisson(maxLam * volume)
    N = generate_grains(maxN, dimX, dimY, dimT, dimV, lamDensity, maxLam)
    return N

def generate_grains(N, dimX, dimY, dimT, dimV, lamDensity, maxLam):
    # precompute and generate N (x,y,t,v,u) tuples where u is the chance we accept this point
    precompute = ((np.random.uniform(*dimX), np.random.uniform(*dimY), \
            np.random.uniform(*dimT), np.random.uniform(*dimV), \
            np.random.uniform(), lamDensity, maxLam) \
            for i in range(N))
    precompute = np.random.rand(N, 5)

    if useThreads:
        pool = Pool()
        grains = list(filter(None, pool.starmap(create_grain_wrapper, precompute)))
    else:
        grains = list(filter(None, (create_grain_wrapper(*pc) for pc in precompute)))

    for grain in grains:
        print(grain)
    return len(grains)

def generate_grain(dimX, dimY, dimT, dimV, lamDensity, maxLam, seed = 0):
    localState = np.random.RandomState(seed) if seed != 0 else np.random

    potentialGrain = (localState.uniform(*dimX), localState.uniform(*dimY), \
            localState.uniform(*dimT), localState.uniform(*dimV))

    return create_grain(*potentialGrain, localState.uniform(), \
            lamDensity, maxLam)

# where x, y, t, v, u are uniform on [0,1) but converted by using dims
def create_grain_wrapper(x, y, t, v, u):
    return create_grain((dimX[1] - dimX[0])*x + dimX[0], \
            (dimY[1] - dimY[0])*y + dimY[0], (dimT[1] - dimT[0])*t + dimT[0], \
            (dimV[1] - dimV[0])*v + dimV[0], u, lamDensity, maxLam)

# determines if the grain should be accepted or not. u is the check for acceptance
def create_grain(x, y, t, v, u, lamDensity, maxLam):
    return Grain(x,y,t,v) if u <= lamDensity(x, y, t, v)/maxLam else None

if __name__ == "__main__":
    # Used for measuring performance
    import time
    import sys

    # print the dimensions
    print("DDDDDDDDDD")
    print("0," + ",".join(repr(i) for i in dimX) + ",W")
    print("1," + ",".join(repr(i) for i in dimY) + ",W")

    # print the grains after ZZZZZZZZZZ
    print("ZZZZZZZZZZ")
    start = time.time()
    N = main(dimX, dimY, dimT, dimV, lamDensity, maxLam)
    end = time.time()

    generationTime = end - start

    print(__file__, "maxLam:", maxLam, file=sys.stderr)
    print(__file__, "generation time:", generationTime, "secs", file=sys.stderr)
    print(__file__, "generated:", N, "points", file=sys.stderr)
