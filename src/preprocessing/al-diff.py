#!/usr/bin/python

# @file al-diff.py
# @brief Compare two different sentence alignments of the same parallel text.
# 
# @author George Foster
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2010, Sa Majeste la Reine du Chef du Canada /
# Copyright 2010, Her Majesty in Right of Canada

# This uses the optparse module (http://docs.python.org/library/optparse.html)
# for argument processing.

import sys
from optparse import OptionParser

usage="al-diff.py [options] al1 al2 [src tgt]"
help="""

Compare two different sentence alignments for a given parallel text. The
alignment files al1 and al2 should be in the format output by ssal, with src as
the 'left-side' text. Output is one line per minimal block of alignments that
differ between al1 and al2. If optional src and tgt files are specified, the
corresponding text regions are written after the alignment difference, src
first. NB: the 'seg' indexes reported for src/tgt won't correspond to line
numbers in these files if -m is given; grep out mark lines to get exact
correspondence.
"""

parser = OptionParser(usage=usage, description=help)
parser.add_option("-m", dest="mark", type="string", default=None,
                  help="interpret MARK, when alone on a line, as hard markup\
                  in src/tgt: paired across both files, and not described by\
                  the alignments (but assumed by them) [%default]")

(opts, args) = parser.parse_args()
if len(args) != 2 and len(args) != 4:
    parser.error("not enough arguments")

al1filename = args[0]
al2filename = args[1]
al1file = file(al1filename)
al2file = file(al2filename)

if len(args) == 4:
    srcfilename = args[2]
    tgtfilename = args[3]
    srcfile = file(srcfilename)
    tgtfile = file(tgtfilename)
else:
    srcfilename = "src"
    tgtfilename = "tgt"
    srcfile = tgtfile = None

# functions

def parseNextAlignment(file, hist, counts):
    s = file.readline()
    if s == "":
        return False;
    toks = s.split()
    al = toks[2].split("-")
    hist.append((int(al[0]),int(al[1])))
    counts[0] += hist[-1][0]
    counts[1] += hist[-1][1]
    return True

def tooShortError(filename):
    print >> sys.stderr, "error: file", filename, "too short!"
    sys.exit(1)

def readLines(file, filename, numlines):
    lines = []
    i = 0
    while i < numlines:
        line = file.readline()
        if line == "":
            tooShortError(filename)
        if opts.mark == None or line != opts.mark + "\n":
           lines.append(line)
           i += 1
    return lines

# main

src_linepos, tgt_linepos = 1, 1

while True:

    hist1, hist2 = [], []
    counts1, counts2 = [0,0], [0,0]

    s1 = parseNextAlignment(al1file, hist1, counts1)
    s2 = parseNextAlignment(al2file, hist2, counts2)

    if s1 != s2:
        tooShortError(al1filename if s2 else al2filename)

    if not (s1 and s2):   # normal end
        break

    while counts1 != counts2:
        if counts1 < counts2:
            if not parseNextAlignment(al1file, hist1, counts1):
                tooShortError(al1filename)
        else:
            if not parseNextAlignment(al2file, hist2, counts2):
                tooShortError(al2filename)

    if srcfile:
        srclines = readLines(srcfile, srcfilename, counts1[0])
        tgtlines = readLines(tgtfile, tgtfilename, counts1[1])

    if hist1 != hist2:
        print hist1, "vs", hist2, " at", \
              srcfilename, "seg", src_linepos, "~", \
              tgtfilename, "seg", tgt_linepos

        if srcfile:
            for l in srclines: print l,
            print "---"
            for l in tgtlines: print l,
            print

    src_linepos += counts1[0]
    tgt_linepos += counts1[1]
