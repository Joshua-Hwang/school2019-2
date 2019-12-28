"""
This module provides classes and functions
to help parse a *.lp file for the
Lilypond model
"""

from collections import namedtuple
from enum import Enum

Collisions = namedtuple("Collisions", ['grain_i', 'grain_j', 'dist'])
# style of collision at the borders
BORDER_STYLES = Enum("BorderStyles", ["Wrapped", "Hard"])
STYLES_DICT = {"W": BORDER_STYLES.Wrapped, "H": BORDER_STYLES.Hard}

class Grain:
    """Class that holds all information about a Grain"""
    def __init__(self, ID, x, y, t, v, r, i):
        """Create a new instance of the Grain class

        Keyword arguments:
        ID -- Usually taken as the nth grain parsed in the file
        x -- The location of the centre of the grain in the x dimension
        y -- The location of the centre of the grain in the y dimension
        t -- The birth time of the grain
        v -- The constant growth rate of the grain
        r -- The final radius of the grain. If -1 it's not born, if 0 it suffocated.
        i -- The ID of the grain it collided with. Usually empty and useless unless
             r is specified."""
        self.id = ID
        self.x = x
        self.y = y
        self.t = t
        self.v = v
        self.r = r
        self.i = i
        self.life_time = self.r/self.v
        self.death_time = self.life_time + t
        self.set = {self}
        # line generation happens in visualise()

    def __hash__(self):
        return hash(self.id)

    def __eq__(self, other):
        """Comparison occurs by comparing the self.id of each grain"""
        if not isinstance(other, type(self)):
            return NotImplemented
        return self.id == other.id

    def __repr__(self):
        return "%f,%f,%f,%f,%f,%d" % (self.x, self.y, self.t, self.v, self.r, self.i)

def parse_all(reader):
    """
    Expects a csv.reader but I think other readers work too
    returns a dictionary which has all the pieces it could parse
    """
    ret = {}

    c = next(reader)[0]
    while c:
        if c.startswith("D"):
            # dimensions section
            dims, c = parse_dims(reader)
            ret["dims"] = dims
        elif c.startswith("Z"):
            # grains section
            grains, c = parse_lps2(reader)
            ret["grains"] = grains
        elif c.startswith("X"):
            # extensions section
            collisions, c = parse_extensions(reader, grains)
            ret["collisions"] = collisions
        else:
            break

    return ret

def parse_lps(reader):
    """
    Expects a csv.reader but I think other readers work too
    """
    return parse_lps2(reader)[0]

def parse_lps2(reader):
    """
    Expects a csv.reader but I think other readers work too
    returns a list of circles along with the character for the next segment
    """
    grains = []
    n = 0
    c = None
    try: # parsing
        for row in reader:
            # ignore comments
            if row[0].startswith('#'):
                continue
            # Alphabet characters are used to halt parsing
            if row[0].isalpha():
                c = row[0][0]
                break
            x, y, t, v, r, i = row # see example.lp for these values
            # perform a very forgiving parse of the file
            x, y, t, v, r, i = float(x), float(y), \
                    abs(float(t)), abs(float(v)), \
                    abs(float(r)), int(i)
            grain = Grain(n, x, y, t, v, r, i)
            grains.append(grain)
            n += 1
    except ValueError:
        raise ValueError("Parsing error has occurred in at %s" % row[0])
    return grains, c

def parse_dims(reader):
    """
    Expects a csv.reader but I think other readers work too
    Assuming the very next line is about dimensions.
    returns a dictionary containing min_Z, max_Z and style_Z for each
    dimension Z
    """
    ret = {}
    c = None
    try: # parsing
        for row in reader:
            # Testing is hard for a list of strings so we'll just concatenate
            tester = ",".join(row)
            # ignore comments
            if tester.startswith('#'):
                continue
            # Alphabet characters are used to halt parsing
            if tester == len(tester) * tester[0] and tester.isalpha():
                if tester[0] == 'D':
                    # We've hit the same header as last time
                    continue
                else:
                    c = tester[0]
                    break
            dim, bmin, bmax, style = row
            # check style is known
            ret["min_" + dim] = float(bmin)
            ret["max_" + dim] = float(bmax)
            ret["style_" + dim] = STYLES_DICT[style]

    except ValueError:
        raise ValueError("Parsing error has occurred in at %s" % ",".join(row))

    return ret, c


def parse_extensions(reader, grains):
    """
    Expects a csv.reader but I think other readers work too
    """
    # collisions are made from tuples of the form
    # (grain_i, grain_j, remaining_dist)
    collisions = []
    c = None
    try: # parsing
        for row in reader:
            # Testing is hard for a list of strings so we'll just concatenate
            tester = ",".join(row)
            # ignore comments
            if row[0].startswith('#'):
                continue
            # Alphabet characters are used to halt parsing
            if tester == len(tester) * tester[0] and tester.isalpha() \
                    and tester[0] != 'X':
                c = tester[0]
                break
            i, j, d = row # see example.lp for these values
            # perform a very forgiving parse of the file
            i, j, d = abs(int(i)), abs(int(j)), abs(float(d))
            collision = Collisions(grains[i], grains[j], d)
            collisions.append(collision)
    except ValueError:
        raise ValueError("Parsing error has occurred in at %s" % row[0])

    collisions.sort(key=lambda x: x[2], reverse=True)
    return collisions, c
        
