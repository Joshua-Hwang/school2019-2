import numpy as np
from matplotlib import pyplot as plt
from matplotlib import animation
import itertools
import math

from gi.repository import Notify # not important just for notification in linux
Notify.init("sim.py job")

lam = 1 # lambda
dim = (0, 10)
tol = 0.01 # tolerance for floating point

maxdim = np.diff(dim).item()

class Circle:
    def __init__(self, xy, v):
        self.xy = xy
        self.r = -1
        self.v = v
        self.shape = plt.Circle(self.xy, 0.01)

def main():
    ### Remove if needed
    """
    for i in range(10):
        Notify.Notification.new("Job starting", "beginning task").show()
        """
    ###

    fig = plt.figure()
    ax = plt.axes(xlim=dim, ylim=dim)

    # generate the number of circles in the area.
    N = np.random.poisson(lam * maxdim**2)
    circles = [Circle(np.random.uniform(*dim, 2), np.random.uniform()/100) for i in range(N)]
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
        return artistShapes

    anim = animation.FuncAnimation(fig, animate, init_func=init,
            frames=math.ceil(maxdim*200), interval=1, blit=True)

    plt.show()

def solve_radii(circles):
    # though not pythonic probably cleanest way of doing this
    # the best I can come up with is a n^4 algo. Take every permutation of 2 (m=n^2) then
    # find the minimum in the set and evaluate it (m) but repeat
    # the search for a new minimum with the new evaluation (m^2). Thus n^4.
    allPerms = list(itertools.permutations(circles, 2))
    for n in range(len(circles)):
        # search for new minimum
        changei = -1
        newMin = -1
        minTime = -1
        for i in range(len(allPerms)):
            if allPerms[i] and allPerms[i][0].r < 0: # is not null and doesn't already have a radius
                circlej = allPerms[i][0]
                circlek = allPerms[i][1]
                dist = np.linalg.norm(circlej.xy - circlek.xy)
                newr = (dist * circlej.v/(circlej.v + circlek.v)) if circlek.r < 0 else (dist - circlek.r) # potential new radius
                t = newr/circlej.v
                if changei >= 0:
                    # check if minTime is larger than t
                    if minTime > t:
                        changei = i
                        newMin = newr
                        minTime = t
                else:
                    changei = i
                    newMin = newr
                    minTime = t
            else:
                allPerms[i] = None
        if changei >= 0:
            allPerms[changei][0].r = newMin

if __name__ == "__main__":
    main()
