#!/usr/bin/env python3
from bisect import bisect_left
import numpy as np
from scipy.integrate import tplquad # we use this to integrate for higher dimensions this must be changed
from matplotlib import pyplot as plt
from matplotlib import animation
import itertools
import math
import time

from gi.repository import Notify # not important just for notification in linux
Notify.init("sim.py job")

# lamd is lambda density
#lamd = lambda x, y, t: np.exp(-1/2*(x**2 + y**2)) * 10
#lamd = lambda x, y, t: np.sin(x)**2/10
lamd = lambda x, y, t: 1/2
maxlam = 1 # a maximum lambda for our non homogeneous lam function
dims = ((-10, 10), (-2, 2), (0, 10)) # where we have the x dimensions first
tol = 0.01 # tolerance for floating point

# [DEBUG] Give every circle a unique id
INDEX = 0
class Circle:
    def __init__(self, x, y, t, v):
        self.xy = (x, y)
        self.t = t
        self.v = v
        self.r = -1
        self.shape = plt.Circle(self.xy, 0.01)

        global INDEX
        self.id = INDEX
        INDEX += 1
    def __repr__(self):
        return "Circle:{id: %d, x: %f, y: %f, t: %f, v: %f, r: %f}" % (self.id, *self.xy, self.t, self.v, self.r)

def main(run = 1):
    ### Remove if needed
    try:
        Notify.Notification.new("Job starting", "beginning task").show()
    except Exception:
        pass
    ###

    fig = plt.figure()
    ax = plt.axes(xlim=dims[0], ylim=dims[1])

    # generate the number of circles in the area.
    lam = tplquad(lamd, dims[2][0], dims[2][1], lambda y: dims[1][0], lambda y: dims[1][1],
            lambda t, y: dims[0][0], lambda t, y: dims[0][1])[0] # we don't care about error
    N = np.random.poisson(lam)
    # program doesn't handle single grain simulations
    if N <= 1:
        print("Error:", N, "elements is not enough")
        return

    circles = generate_circles(N, dims, lamd, maxlam)

    start = time.process_time()
    t = solve_radii(circles)
    end = time.process_time()
    print("solve_radii completed in", end - start, "seconds with", N, "points")

    # [DEBUG] check for radii of -1 because there shouldn't be any
    errors = [circle for circle in circles if circle.r < 0]
    if errors:
        print("Errors at run", run)
        ### Remove if needed
        try:
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
            Notify.Notification.new("Job closing", "Task has failed").show()
        except Exception:
            pass
        ###
    else:
        print("No errors at run", run)
        main(run+1)

#    artistShapes = [circle.shape for circle in circles] # needed for init() and animate()
#
#    def init():
#        for circle in circles:
#            ax.add_patch(circle.shape)
#        return artistShapes
#
#    def animate(i):
#        for circle in circles:
#            # ignore circle if radius <= 0 (never born) or birth time hasn't happened yet
#            r = circle.v * (i - circle.t)
#            if r < circle.r:
#                circle.shape.set_radius(max(r,0))
#            else:
#                circle.shape.set_radius(circle.r)
#        return artistShapes
#
#    anim = animation.FuncAnimation(fig, animate, init_func=init,
#            frames=math.ceil(t) + 100, interval=1, blit=True)
#
#    plt.axis('scaled')
#    plt.show()

def generate_circles(N, dims, lamd, maxlam):
    n = 0
    circles = np.empty(N, dtype=object)
    while n < N:
        # ensure first point is at time 0
        # [DEPRC] maybe we no longer need to set our first birth at t=0?
        #tmp = (np.random.uniform(*dims[0]), np.random.uniform(*dims[1]), np.random.uniform(*dims[2]) if n > 0 else 0)
        tmp = (np.random.uniform(*dims[0]), np.random.uniform(*dims[1]), np.random.uniform(*dims[2]))
        if np.random.uniform(0,1) <= lamd(*tmp)/maxlam:
            # accept point
            #circles[n] = Circle(*tmp, np.random.uniform()/100)
            circles[n] = Circle(*tmp, 0.01)
            n = n + 1

    return circles

# A potential way of reducing the factor of n^4 is to only change the pairings where that circle is involved then sort this
# nearly sorted list of results.

# returns the global time
def solve_radii(circles):
    # though not pythonic probably cleanest way of doing this
    # the best I can come up with is a n^4 algo. Take every permutation of 2 (m=n^2) then
    # find the minimum in the set and evaluate it (m) but repeat
    # the search for a new minimum with the new evaluation (m^2). Thus n^4.
    allPerms = list(itertools.permutations(circles, 2))
    gt = 0 # this is the global time which we use to account for births
    bts = sorted([circle.t for circle in circles]) # bt is for birth times
    n = 0
    while n <= len(circles) and any(allPerms):
        print("Progress: Evaluating", n, "of", len(circles), "at", gt, "frames", end = "\n")
        # search for new minimum
        changei = -1 # the index we intend to change
        evalr = -1 # the radius we evaluated
        minTime = -1 # the time when this occurs
        for i in range(len(allPerms)):
            cPerm = allPerms[i] # use a short name for current permutation
            if cPerm and cPerm[0].r < 0 and cPerm[1].r != 0: # is not null and doesn't already have a radius and isn't being paired with a stillborn grain
                circlej = cPerm[0]
                circlek = cPerm[1]
                if circlej.t > gt: # check if our circlej is still not born also check if we grew we don't suffocate the partner
                    continue

                dist = np.linalg.norm(np.array(circlej.xy) - np.array(circlek.xy))

                if (dist/circlej.v) < (circlek.t - circlej.t): # circlej will arrive before circlek gets born
                    # Trying to drop None means we have to change the indices
                    print("Dropping:", allPerms[i])
                    allPerms[i] = None
                    continue

                t = (dist + circlej.v * circlej.t + circlek.v * circlek.t)/(circlej.v + circlek.v) if circlek.r < 0 else (dist + circlej.v * circlej.t - circlek.r)/(circlej.v)
                if t < gt: # if t has time less than global time then our grain is inside another we should kick it out, set radius to 0
                    circlej.r = 0
                    n += 1
                    print("Evaluated circle:", circlej)
                    continue
                newr = circlej.v * (t - circlej.t) # potential new radius
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
                # Trying to drop None means we have to change the indices
                allPerms[i] = None # we don't need to look at this pairing ever again
        if changei >= 0: # we evaluate a time step
            allPerms[changei][0].r = evalr
            gt = max(gt, minTime)
            n += 1
            print("Evaluated circle:", allPerms[changei])
        else: # this either means I have an off by one error OR someone has a very far off birth time
            print("Failed to evaluate anything")
            gt = bts[bisect_left(bts, gt)] if gt < bts[-1] else gt
    print("Completed:", n, "of", len(circles), "with", gt, "frames", end = "\n")
    print("remaining pairs:", [pair for pair in allPerms if pair])
    print("result:", circles)
    return gt

if __name__ == "__main__":
    main()
