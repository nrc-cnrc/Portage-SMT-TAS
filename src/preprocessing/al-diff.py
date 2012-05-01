#!/usr/bin/env python

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

########################################
# How to visualize alignment differences with vim:
#
# Add the following to your .vimrc
#    function DiffAligment()
#       silent execute "!" . $AL_DIFF . " " . $AL1 . " " . $AL2 . " " . v:fname_in . " " . v:fname_new . " > " . v:fname_out
#    endfunction
#
# Add the following function to your aliases.
#    vimdiffal() {
#       AL_DIFF=${AL_DIFF:-"al-diff.py -v -m '<PORTAGE_DOCUMENT_END>'"} AL1=$3 AL2=$4 vimdiff +"set diffexpr=DiffAligment()" +"set diffopt=filler,context:0" $1 $2
#    }
#
# Then simply type on the command line:
#    vimdiffal source target alignment.1 alignment.2


import sys
import gzip
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
parser.add_option("-v", dest="vimMode", action="store_true", default=False, \
                  help="Changes the output to a diff style output.")

(opts, args) = parser.parse_args()
if len(args) != 2 and len(args) != 4:
    parser.error("not enough arguments")

def openfile(filename):
   if (filename.endswith(".gz")):
      return gzip.open(filename, "rb")
   else:
      return file(filename)


al1filename = args[0]
al1file = openfile(al1filename)

al2filename = args[1]
al2file = openfile(al2filename)

if len(args) == 4:
    srcfilename = args[2]
    srcfile = openfile(srcfilename)
    tgtfilename = args[3]
    tgtfile = openfile(tgtfilename)
else:
    srcfilename = "src"
    tgtfilename = "tgt"
    srcfile = tgtfile = None


if opts.vimMode:
    if len(args) != 4:
        print >> sys.stderr, "When using -v, you must also provide src & tgt!"
        sys.exit(1)

    # Special case for stupid vim check
    a = open(srcfilename, "r")
    ahead = a.readline()
    b = open(tgtfilename, "r")
    bhead = b.readline()
    if (ahead == "line1\n" and bhead == "line2\n" and not a.readline() and not b.readline()):
        # sys.stderr.write('Found VIM\n')  # SAM DEBUGGING
        print "1c1"
        sys.exit(0)
    a.close()
    b.close()


# functions

def parseNextAlignment(file, hist, counts):
    s = file.readline()
    if opts.mark != None and s == opts.mark + "\n":
        s = file.readline()
    if s == "":
        return False
    toks = s.split()
    al = toks[2].split("-")
    hist.append((int(al[0]),int(al[1])))
    counts[0] += hist[-1][0]
    counts[1] += hist[-1][1]
    return True

def tooShortError(filename):
    print >> sys.stderr, "error: file", filename, "too short!"
    sys.exit(1)

def readLines(file, filename, numlines, linepos):
    lines = []
    i = 0
    while i < numlines:
        line = file.readline()
        if line == "":
            tooShortError(filename)
        if opts.mark == None or line != opts.mark + "\n":
           lines.append(line)
           i += 1
        if opts.vimMode and opts.mark != None and line == opts.mark + "\n":
           linepos += 1
    return lines, linepos

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
        srclines, src_linepos = readLines(srcfile, srcfilename, counts1[0], src_linepos)
        tgtlines, tgt_linepos = readLines(tgtfile, tgtfilename, counts1[1], tgt_linepos)

    if hist1 != hist2:
        if opts.vimMode:
            print '{},{}c{},{}'.format(src_linepos, src_linepos+counts1[0]-1, tgt_linepos, tgt_linepos+counts1[1]-1)
        else:
            print hist1, "vs", hist2, " at", \
                  srcfilename, "seg", src_linepos, "~", \
                  tgtfilename, "seg", tgt_linepos

        if srcfile:
            if opts.vimMode:
                for l in srclines: print '< {}'.format(l),
                print "---"
                for l in tgtlines: print '> {}'.format(l),
            else:
                for l in srclines: print l,
                print "---"
                for l in tgtlines: print l,
                print

    src_linepos += counts1[0]
    tgt_linepos += counts1[1]
