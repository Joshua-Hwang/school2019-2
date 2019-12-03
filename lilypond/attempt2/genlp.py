#!/usr/bin/env python3
import numpy as np
from scipy.integrate import nquad # we use this to integrate for higher dimensions this must be changed
import imageio
import math
from multiprocessing import Pool
from functools import partial

# lam is short for the Poisson parameter lambda (not the functional thing)
def lamDensity(x, y, t, v):
    #return 1/2
    #return 1/(t+1)**2 * np.sin(x)**2 * 10
    return 1/(t+1) * np.exp(-1/2*(x**2 + y**2)) * 10

# numerical integration is slow. Precompute for speed. None for numeric.
maxLam = 27.547774551166047

dimX = (-20, 20)
dimY = (-20, 20)
dimT = (0, 100)
dimV = (0.005, 0.1)

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
def main(dimX, dimY, dimT, lamDensity, maxLam):
    # single grain simulation is boring
    N = np.random.poisson(maxLam)
    generate_grains(N, dimX, dimY, dimT, lamDensity, maxLam)
    return N

def generate_grains(N, dimX, dimY, dimT, lamDensity, maxLam = None):
    if maxLam is None:
        maxLam, _ = nquad(lamDensity, [dimX, dimY, dimT, dimV])

    pool = Pool()
    grains = pool.starmap(generate_grain,
            ((dimX, dimY, dimT, lamDensity, maxLam, i) for i in range(N)))
    for grain in grains:
        print(grain)

def generate_grain(dimX, dimY, dimT, lamDensity, maxLam, seed = 0):
    localState = np.random.RandomState(seed)
    while True:
        potentialGrain = (localState.uniform(*dimX), localState.uniform(*dimY), \
                localState.uniform(*dimT), localState.uniform(*dimV))
        if localState.uniform(0,1) <= lamDensity(*potentialGrain)/maxLam:
            return Grain(*potentialGrain)

if __name__ == "__main__":
    # Used for measuring performance
    import time
    import sys

    start = time.time()
    if maxLam is None:
        maxLam, _ = nquad(lamDensity, [dimX, dimY, dimT, dimV]) # '_' is the numerical error
    end = time.time()
    nquadTime = end - start

    start = time.time()
    N = main(dimX, dimY, dimT, lamDensity, maxLam)
    end = time.time()
    generationTime = end - start

    print(__file__, "nquad time:", nquadTime, "secs", file=sys.stderr)
    print(__file__, "maxLam:", maxLam, file=sys.stderr)
    print(__file__, "generation time:", generationTime, "secs", file=sys.stderr)
    print(__file__, "generated:", N, "points", file=sys.stderr)
