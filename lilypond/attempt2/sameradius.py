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

    def __hash__(self):
        return hash((self.x, self.y))

    # If they're in the same position we need to get rid of them
    def __eq__(self, other):
        if not isinstance(other, type(self)):
            return NotImplemented
        diff = abs(self.x - other.x) + abs(self.y - other.y)
        return diff < tol

    def __repr__(self):
        return "%f,%f,%f,%f,%f,%d" % (self.x, self.y, self.t, self.v, self.r, self.i)

def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith('#'))

    circles = set()
    try: # parsing
        for row in reader:
            # Alphabet characters are used to halt parsing
            if row[0].isalpha():
                break
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
