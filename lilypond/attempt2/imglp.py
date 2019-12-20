#!/usr/bin/env python3
import numpy as np
from scipy.integrate import nquad # we use this to integrate for higher dimensions this must be changed
import imageio
import math
from multiprocessing import Pool
from functools import partial
from PIL import Image, ImageOps

import argparse

parser = argparse.ArgumentParser(description="Takes an input of an image generates points based on blackness of pixels")
parser.add_argument("inputimage", help="if specified reads an otherwise standard in")

useThreads = True

# numerical integration is slow. Precompute for speed. None for numeric.
maxLam = 0.01

dimX = (0, 0)
dimY = (0, 0)
V = 0.1 # velocity for all grains

pix = None
# lam is short for the Poisson parameter lambda (not the functional thing)
def lamDensity(x, y):
    # round x and y to nearest integer
    x = int(x)
    y = int(y)
    # since row and column
    return pix[y, x]

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
def main(dimX, dimY, lamDensity, maxLam):
    # single grain simulation is boring
    volume = (np.diff(dimX) * np.diff(dimY))[0]
    maxN = np.random.poisson(maxLam * volume)
    N = generate_grains(maxN, dimX, dimY, lamDensity, maxLam)
    return N

def generate_grains(N, dimX, dimY, lamDensity, maxLam):
    grains = []
    if useThreads:
        pool = Pool()
        grains = list(filter(None, pool.starmap(generate_grain,
                ((dimX, dimY, lamDensity, maxLam, i) for i in range(N)))))
    else:
        grains = list(filter(None,
                (generate_grain(dimX, dimY, lamDensity, maxLam, i)
                for i in range(N))))
    for grain in grains:
        print(grain)
    return len(grains)

def generate_grain(dimX, dimY, lamDensity, maxLam, seed = 0):
    localState = np.random.RandomState(seed)
    potentialGrain = (localState.uniform(*dimX), localState.uniform(*dimY))
    if localState.uniform(0,1) <= lamDensity(*potentialGrain)/maxLam:
        return Grain(*potentialGrain, 0, V)
    else:
        return None

if __name__ == "__main__":
    args = parser.parse_args()
    pic = Image.open(args.inputimage).convert('L').transpose(Image.FLIP_TOP_BOTTOM)
    pic = ImageOps.invert(pic)
    pix = np.array(pic)/255 * maxLam
    dimX = (0, pix.shape[1])
    dimY = (0, pix.shape[0])

    # Used for measuring performance
    import time
    import sys

    start = time.time()
    N = main(dimX, dimY, lamDensity, maxLam)
    end = time.time()
    generationTime = end - start

    print(__file__, "maxLam:", maxLam, file=sys.stderr)
    print(__file__, "generation time:", generationTime, "secs", file=sys.stderr)
    print(__file__, "generated:", N, "points", file=sys.stderr)
