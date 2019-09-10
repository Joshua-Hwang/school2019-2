#!/usr/bin/env python3
import math
import random
random.seed(44302650) # My student number

N = 10**3

# realise initial state X0

def initial():
    res = random.random()
    if res < 1/6:
        return -1
    elif res < 1/2:
        return 0
    else:
        return 1

# take a single step in this chain where X is the lastest state
def step(X):
    res = random.random()
    if X == -1:
        if res < 1/6:
            return -1
        else:
            return 0
    elif X == 0:
        if res < 1:
            return 1
        else:
            return 1
    else:
        if res < 1/3:
            return -1
        else:
            return 0

res = [step(step(initial())) for i in range(N)]

print("==================================================")
# first part
pi = sum(((X == 1) for X in res))/N
pise = math.sqrt(1/(N-1)*sum(((X == 1) - pi)**2 for X in res))/math.sqrt(N)
#pise = math.sqrt(pi*(1-pi)/(N-1))/math.sqrt(N)
print("P(X_2=1) ~ {}".format(pi))
print("standard error = {}".format(pise))
print("P(X_2=1) = {}".format(17/36))

print("--------------------------------------------------")

# second part
mu = sum((X for X in res))/N
muse = math.sqrt(sum((X - mu)**2 for X in res)/(N-1))/math.sqrt(N)
print("E[X_2] ~ {}".format(mu))
print("standard error = {}".format(muse))
print("E[X_2] = {}".format(71/216))

print("--------------------------------------------------")

# third part
s2 = sum((X - mu)**2 for X in res)/(N-1)
s2se = math.sqrt(sum(((X - mu)**2 - s2)**2 for X in res)/(N-1))/math.sqrt(N)
print("Var(X_2) ~ {}".format(s2))
print("standard error = {}".format(s2se))
print("Var(X_2) = {}".format(23687/46656))
print("==================================================")
