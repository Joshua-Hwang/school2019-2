#!/usr/bin/env python3
import sys
import csv
import math

import collections
import argparse
import fileinput

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and sets all radii to the same value")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

R = 1

class Circle:
    def __init__(self, x, y, t, v, r, i):
        self.x = x
        self.y = y
        self.t = t
        self.v = v
        self.r = r
        self.i = i
        # line generation happens in visualise()

    def __repr__(self):
        return "%f,%f,%f,%f,%f,%d" % (self.x, self.y, self.t, self.v, self.r, self.i)

def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith("#"))

    circles = set()
    try: # parsing
        for row in reader:
            x, y, t, v, r, i = row # see example.lp for these values
            # perform a very forgiving parse of the file
            x, y, t, v, r, i = float(x), float(y), \
                    abs(float(t)), abs(float(v)), \
                    abs(float(r)), abs(int(i))
            circle = Circle(x, y, t, v, R, i)
            circles.add(circle)
    except ValueError as e:
        raise ValueError("Parsing error has occurred in %s at %s" % (fileinput.filename(), row[0]))

    for circle in circles:
        print(circle)

if __name__ == "__main__":
    main()
