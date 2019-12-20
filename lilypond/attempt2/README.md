This document defines the \*.lp (short for lilypond) format used
to construct a realisation of the lilypond model.

For the most part the an lp file is a bunch of concatenated csv files.
Each section is split by a line (of at least 5 or more) alphabetical and
identical characters (case sensitive).

The character used to create the system is unique for each section.
For example the "XXX" is to denote a section filled with extensions
following the Penrose & Last (2011) extension of the model. ZZZ defines
the section that contains information about each grain.

If the same section occurs twice in the document it is up to each
section to define how it handles this.

