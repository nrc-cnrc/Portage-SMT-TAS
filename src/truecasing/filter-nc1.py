#!/usr/bin/env python

# @file filter-nc1.py
# @brief Filter out words up to the first cased word inclusive (if title cased)
#        for building an nc1 language model.
# 
# @author Darlene Stewart
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2011, Sa Majeste la Reine du Chef du Canada /
# Copyright 2011, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
import codecs
import re
from argparse import ArgumentParser, FileType
import os
import os.path
import gzip

# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path).
if sys.argv[0] not in ('', '-c'):
    bin_path = os.path.dirname(sys.argv[0])
    if os.path.basename(bin_path) != "bin":
        sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

from portage_utils import *

printCopyright("filter-nc1.py", 2011);
os.environ['PORTAGE_INTERNAL_CALL'] = '1';


usage="filter-nc1.py [options] [infile [outfile]]"
help = """
Filter out words up to the first cased word inclusive (if title cased) in 
<infile> for the purpose of producing text for building an nc1 language model.
"""

parser = ArgumentParser(usage=usage, description=help, add_help=False)
parser.add_argument("-h", "-help", "--help", action=HelpAction)
parser.add_argument("-v", "--verbose", action=VerboseAction)
parser.add_argument("-d", "--debug", action=DebugAction)
parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                    help="file encoding [%(default)s]")
parser.add_argument("infile", nargs='?', type=FileType('r'), default=sys.stdin, 
                    help="input file of text to add/remove case markup to")
parser.add_argument("outfile", nargs='?', type=FileType('w'), default=sys.stdout, 
                    help="output text file with case markup added/removed")


def first_upper(s):
    """Return the index of the first uppercase character in string s."""
    for idx, c in enumerate(s):
        if c.isupper():
            return idx
    return -1

def first_lower(s):
    """Return the index of the first lowercase character in string s."""
    for idx, c in enumerate(s):
        if c.islower():
            return idx
    return -1

def first_cased(s):
    """Return the index of the first cased character in string s."""
    for idx, c in enumerate(s):
        if c.islower() or c.isupper():
            return idx
    return -1

def is_cased(s):
    """Return True if string s contains some cased characters; otherwise false."""
    return s.lower() != s.upper()

integer_re = re.compile('(?:-?\d?\d?\d(?:,\d\d\d)+$)|(?:-?\d+$)')
def is_integer(s):
    if s.isnumeric():
        return True
    return integer_re.match(s) != None

number_re = re.compile('(?:-?\d?\d?\d(?:,\d\d\d)+\.?\d*$)|(?:-?\d+\.?\d*$)|(?:-?\.?\d+$)')
def is_number(s):
    if s.isnumeric():
        return True
    return number_re.match(s) != None

time_re = re.compile('(?:(?:[0-1]?[0-9]|2[0-3]):[0-5][0-9]$)')
def is_time(s):
    return time_re.match(s) != None

def main():
    cmd_args = parser.parse_args()

    if cmd_args.infile.name.endswith('.gz'):
        cmd_args.infile.close()
        cmd_args.infile = gzip.open(cmd_args.infile.name, 'rb')
    if cmd_args.outfile.name.endswith('.gz'):
        cmd_args.outfile.close()
        cmd_args.outfile = gzip.open(cmd_args.outfile.name, 'wb')
    
    try:
        codecs.lookup(cmd_args.encoding)
    except LookupError:
        fatal_error("Illegal encoding specified for -enc (--encoding) option: '{0}'".format(cmd_args.encoding))
    
    infile = codecs.getreader(cmd_args.encoding)(cmd_args.infile)
    outfile = codecs.getwriter(cmd_args.encoding)(cmd_args.outfile)
    sys.stderr = codecs.getwriter(cmd_args.encoding)(sys.stderr)  # for  utf-8 debug msgs
    
    for line in infile:
        toks = split(line)
        skip = None
        for i,tok in enumerate(toks):
            if is_cased(tok):
                skip = i
                break
        if skip is None: continue                   # no cased text -> skip line
        prev = toks[skip-1] if skip > 0 else ''
        comma = prev == ',' and skip > 1
        if comma: prev = toks[skip-2]
        if prev in ('%', '$') and skip-comma > 1 and is_number(toks[skip-comma-2]):
            skip -= 2 + comma
        elif is_number(prev) and skip-comma > 1 and toks[skip-comma-2] == '$':
            skip -= 2 + comma
        elif prev == '%' or is_integer(prev) or is_time(prev):
            skip -= 1 + comma
        elif tok.istitle():
            skip += 1
        if skip < len(toks):
            print(" ".join(toks[skip:]), "", file=outfile)
        
    infile.close()
    outfile.close()


if __name__ == '__main__':
    main()
