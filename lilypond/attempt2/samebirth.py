#!/usr/bin/env python3
import sys
import csv
import math

import collections
import argparse
import fileinput

import lp

parser = argparse.ArgumentParser(description="Takes an input of *.lp format and sets all birth times to 0")
parser.add_argument("inputfile", help="if specified reads a *.lp formatted file otherwise standard in")

def main():
    reader = csv.reader(row for row in fileinput.input() if not row.startswith('#'))

    circles = lps.parse_lps(reader)

    for circle in circles:
        circle.t = 0
        print(circle)

if __name__ == "__main__":
    main()
