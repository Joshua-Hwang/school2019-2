#!/usr/bin/env python3
import numpy as np
from scipy.integrate import nquad # we use this to integrate for higher dimensions this must be changed
import imageio
import math
from multiprocessing import Pool
from functools import partial

useThreads = True
# numerical integration is slow. Precompute for speed. None for numeric.
maxLam = 20

dimX = (-20, 20)
dimY = (-20, 20)
dimT = (0, 100)
dimV = (0.005, 0.1)

# lam is short for the Poisson parameter lambda (not the functional thing)
def lamDensity(x, y, t, v):
    return np.exp(-1/2*(x**2 + y**2)) * 10
    return 1/(t+1) * np.exp(-1/2*(x**2 + y**2)) * 10
    return 1/2
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
    volume = (np.diff(dimX) * np.diff(dimY) * np.diff(dimT) * np.diff(dimV))[0]
    maxN = np.random.poisson(maxLam * volume)
    N = generate_grains(maxN, dimX, dimY, dimT, dimV, lamDensity, maxLam)
    return N

def generate_grains(N, dimX, dimY, dimT, dimV, lamDensity, maxLam = None):
    if maxLam is None:
        maxLam, _ = nquad(lamDensity, [dimX, dimY, dimT, dimV])

    grains = []
    if useThreads:
        pool = Pool()
        grains = list(filter(None, pool.starmap(generate_grain,
                ((dimX, dimY, dimT, dimV, lamDensity, maxLam, i) for i in range(N)))))
    else:
        grains = list(filter(None,
                (generate_grain(dimX, dimY, dimT, dimV, lamDensity, maxLam, i)
                for i in range(N))))
    for grain in grains:
        print(grain)
    return len(grains)

def generate_grain(dimX, dimY, dimT, dimV, lamDensity, maxLam, seed = 0):
    localState = np.random.RandomState(seed)
    potentialGrain = (localState.uniform(*dimX), localState.uniform(*dimY), \
            localState.uniform(*dimT), localState.uniform(*dimV))
    if localState.uniform(0,1) <= lamDensity(*potentialGrain)/maxLam:
        return Grain(*potentialGrain)
    else:
        return None

if __name__ == "__main__":
    # Used for measuring performance
    import time
    import sys

    start = time.time()
    N = main(dimX, dimY, dimT, dimV, lamDensity, maxLam)
    end = time.time()
    generationTime = end - start

    print(__file__, "maxLam:", maxLam, file=sys.stderr)
    print(__file__, "generation time:", generationTime, "secs", file=sys.stderr)
    print(__file__, "generated:", N, "points", file=sys.stderr)
