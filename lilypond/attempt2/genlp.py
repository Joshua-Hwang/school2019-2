#!/usr/bin/env python3
import numpy as np
from scipy.integrate import tplquad # we use this to integrate for higher dimensions this must be changed
import math

# lam is short for the Poisson parameter lambda (not the functional thing)
#lamDensity = lambda x, y, t: np.exp(-1/2*(x**2 + y**2)) * 10
#lamDensity = lambda x, y, t: np.sin(x)**2/10
lamDensity = lambda x, y, t: 1/2

dimX = (-10, 10)
dimY = (-2, 2)
dimT = (0, 10)

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
def main():
    lam, _ = tplquad(lamDensity, dimT[0], dimT[1], lambda y: dimY[0], lambda y: dimY[1],
            lambda t, y: dimX[0], lambda t, y: dimX[1]) # '_' is the numerical error

    # single grain simulation is boring
    N = np.random.poisson(lam)
    generate_grains(N, dimX, dimY, dimT, lamDensity, lam)

def generate_grains(N, dimX, dimY, dimT, lamDensity, maxLam = None):
    if maxLam is None:
        maxLam, _ = tplquad(lamDensity, dimT[0], dimT[1], lambda y: dimY[0], lambda y: dimY[1],
                lambda t, y: dimX[0], lambda t, y: dimX[1])

    n = 0
    while n < N:
        potentialGrain = (np.random.uniform(*dimX), np.random.uniform(*dimY), np.random.uniform(*dimT))
        if np.random.uniform(0,1) <= lamDensity(*potentialGrain)/maxLam:
            # accept point
            print(Grain(*potentialGrain, np.random.uniform()/10))
            n += 1


if __name__ == "__main__":
    main()
    # Was used for measuring performance
    #import profile
    #profile.run('main()')
