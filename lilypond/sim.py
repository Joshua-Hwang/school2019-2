#!/usr/bin/env python3
import numpy as np
from scipy.integrate import dblquad # we use this to integrate for higher dimensions this must be changed
from matplotlib import pyplot as plt
from matplotlib import animation
import itertools
import math

from gi.repository import Notify # not important just for notification in linux
Notify.init("sim.py job")

# lamd is lambda density
#lamd = lambda x, y: np.exp(-1/2*(x**2 + y**2))
lamd = lambda x, y: np.sin(x)**2
maxlam = 1 # a maximum lambda for our non homogeneous lam function
dims = ((0, 10), (0, 20)) # where we have the x dimensions first
tol = 0.01 # tolerance for floating point


class Circle:
    def __init__(self, xy, t, v):
        self.xy = xy
        self.r = -1
        self.v = v
        self.t = t
        self.shape = plt.Circle(self.xy, 0.01)

def main():
    ### Remove if needed
    """
    for i in range(10):
        Notify.Notification.new("Job starting", "beginning task").show()
        """
    ###

    fig = plt.figure()
    ax = plt.axes(xlim=dims[0], ylim=dims[1])

    # generate the number of circles in the area.
    lam = dblquad(lamd, dims[1][0], dims[1][1], lambda x: dims[0][0], lambda x: dims[0][1])[0] # we don't care about error
    print("lam:",lam)
    N = np.random.poisson(lam)
    print("N:",N)
    circles = generate_circles(N, dims, lamd, maxlam)
    solve_radii(circles)

    ### Remove if needed
    """
    for i in range(10):
        Notify.Notification.new("Job done", "solve_radii() is complete").show()
        """
    ###

    artistShapes = [circle.shape for circle in circles] # needed for init() and animate()

    def init():
        for circle in circles:
            ax.add_patch(circle.shape)
        return artistShapes

    def animate(i):
        for circle in circles:
            r = i * circle.v
            if r < circle.r:
                circle.shape.set_radius(r)
            else:
                circle.shape.set_radius(circle.r)
        return artistShapes

    anim = animation.FuncAnimation(fig, animate, init_func=init,
            frames=math.ceil(np.diff(dims).max()*200), interval=1, blit=True)

    plt.axis('scaled')
    plt.show()

def generate_circles(N, dims, lamd, maxlam):
    n = 0
    circles = np.empty(N, dtype=object)
    while n < N:
        tmp = (np.random.uniform(*dims[0]), np.random.uniform(*dims[1]))
        if np.random.uniform(0,1) <= lamd(*tmp)/maxlam:
            # accept point
            circles[n] = Circle(tmp, 0, np.random.uniform()/100)
            n = n + 1

    return circles

def solve_radii(circles):
    # though not pythonic probably cleanest way of doing this
    # the best I can come up with is a n^4 algo. Take every permutation of 2 (m=n^2) then
    # find the minimum in the set and evaluate it (m) but repeat
    # the search for a new minimum with the new evaluation (m^2). Thus n^4.
    allPerms = list(itertools.permutations(circles, 2))
    for n in range(len(circles)):
        # search for new minimum
        changei = -1 # the index we intend to change
        evalr = -1 # the radius we evaluated
        minTime = -1 # the time when this occurs
        for i in range(len(allPerms)):
            if allPerms[i] and allPerms[i][0].r < 0: # is not null and doesn't already have a radius
                circlej = allPerms[i][0]
                circlek = allPerms[i][1]
                dist = np.linalg.norm(np.array(circlej.xy) - np.array(circlek.xy))
                newr = (dist * circlej.v/(circlej.v + circlek.v)) if circlek.r < 0 else (dist - circlek.r) # potential new radius
                t = newr/circlej.v
                if changei >= 0:
                    # check if minTime is larger than t
                    if minTime > t:
                        changei = i
                        evalr = newr
                        minTime = t
                else:
                    changei = i
                    evalr = newr
                    minTime = t
            else:
                allPerms[i] = None
        if changei >= 0:
            allPerms[changei][0].r = evalr

if __name__ == "__main__":
    main()
