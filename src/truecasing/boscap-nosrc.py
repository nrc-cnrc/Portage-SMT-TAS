#!/usr/bin/env python
# coding=utf-8

# @file boscap-nosrc.py
# @brief Apply beginning-of-sentence (BOS) capitalization.
#
# This script takes into account sentence-break markers to determine the
# BOS capitalization points.
#
# This is accomplished by: first adding BOS markup to the truecased target text
# and then removing the BOS markup from the truecased target language text,
# applying the indicated BOS case as appropriate.
# 
# @author Darlene Stewart
# 
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2012, Sa Majeste la Reine du Chef du Canada /
# Copyright 2012, Her Majesty in Right of Canada

from __future__ import print_function, unicode_literals, division, absolute_import

import time
start_time = time.time()

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

printCopyright("boscap-nosrc.py", 2012);
os.environ['PORTAGE_INTERNAL_CALL'] = '1';


usage = "boscap-nosrc.py [options] tgtfile [outfile]"
help = """
Add beginning-of-sentence (BOS) capitalization to truecased target language
text by examining the sentence break markers in the target language text.
"""

parser = ArgumentParser(usage=usage, description=help, add_help=False)
parser.add_argument("-h", "-help", "--help", action=HelpAction)
parser.add_argument("-v", "--verbose", action=VerboseAction)
parser.add_argument("-d", "--debug", action=DebugAction)
parser.add_argument("-time", "--time", action='store_true', default=False,
                    help="provide timing stats [%(default)s]")
parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                    help="file encoding [%(default)s]")
parser.add_argument("-tgtbos", "--tgtbos", nargs='?', type=FileType('w'), default=None,
                    help="output intermediate file of target text with BOS markup [%(default)s]")
parser.add_argument("-u", "--unk", choices=("title-basic","title-all"), dest="unk_case", default="title-all",
                    help="casing strategy to apply for unknown BOS case (unk) [%(default)s]")

parser.add_argument("tgtfile", type=FileType('r'), 
                    help="truecased target language text file to capitalize BOS in")

parser.add_argument("outfile", nargs='?', type=FileType('w'), default=sys.stdout, 
                    help="truecased target language text with BOS capitalization")

cmd_args = parser.parse_args()
title_unk_case = cmd_args.unk_case.startswith("title")
title_unk_ext_case = cmd_args.unk_case == "title-all"


def first_upper(s):
    """Return the index of the first uppercase character in string s, 
    or -1 if none found.
    """
    for idx, c in enumerate(s):
        if c.isupper():
            return idx
    return -1

def first_lower(s):
    """Return the index of the first lowercase character in string s, 
    or -1 if none found.
    """
    for idx, c in enumerate(s):
        if c.islower():
            return idx
    return -1

def first_cased(s):
    """Return the index of the first cased character in string s, 
    or -1 if no cased character found.
    """
    for idx, c in enumerate(s):
        if c.islower() or c.isupper():
            return idx
    return -1

def is_cased(s):
    """Return True if string s contains some cased characters; otherwise false."""
    return s.lower() != s.upper()

def capitalize_token(s):
    """Capitalize the first cased letter in a word, treating initials specially.
    
    capitalize_token('a.b.c.') returns 'A.B.C.'
    capitalize_token('a.bb') returns 'A.bb'
    capitalize_token('a/b/c') returns 'A/B/C'
    capitalize_token('aa/bb/cc') returns 'Aa/bb/cc'
    
    s: single word string to be capitalized
    returns: capitalized version of s
    """
    def _cap(s):
        fl = first_lower(s)
        if fl is -1:                # no lowercase
            return s
        fu = first_upper(s)
        if fu >= 0 and fu < fl:     # already capitalized
            return s
        return s[:fl] + s[fl].upper() + s[fl+1:]
    def _capitalize_token(s, init_only=False):
        if s == '': return s
        part = s.partition('/')
        if part[1] == '':
            part = s.partition('.')
        if len(part[0]) > 1:
            return s if init_only else _cap(s)
        else:
            return _cap(part[0]) + part[1] + _capitalize_token(part[2], True)
    return _capitalize_token(s)

dashes = ('-', '–', '—')
extended_eos_punc = ((':',) + dashes)
eos_punc = ('.', '!', '?') + extended_eos_punc

number_re = re.compile('(?:-?\d?\d?\d(?:,\d\d\d)+\.?\d*$)|(?:-?\d+\.?\d*$)|(?:-?\.?\d+$)')
def is_number(s):
    """Return True if the string contains a number, e.g. 35, 35., -1.35, .35"""
    if s.isnumeric():
        return True
    return number_re.match(s) != None

sec_num_re = re.compile('\d+(?:\.\d+)+')
def is_section_number(s):
    """Return True if the string contains a section number, e.g. 1.12.3"""
    return sec_num_re.match(s) != None

numeric_re = re.compile('[\d:,.–—\-+/]*\d[\d:,.–—\-+/]*$')
def is_numeric(s):
    """Return True if the string contains numeric content and corresponding punctuation."""
    if s.isnumeric():
        return True
    return numeric_re.match(s) != None

time_re = re.compile('(?:(?:[0-1]?[0-9]|2[0-3]):[0-5][0-9]$)')
def is_time(s):
    """Return True if the string contains a time, e.g. 15:35"""
    return time_re.match(s) != None

def add_tgt_bos_markup(tgt_line, line_number):
    """Add beginning-of-sentence XML markup to a sentence in the target language text.
    
    <bos case="..."/> elements are preprended to tokens as necessary.
    
    tgt_line: line of target language text to mark up
    line_number: line number of the sentence (for error messages)
    returns: target language text with BOS markup added
    """
    tgt_toks = split(tgt_line)
    
    # Add BOS case markup as needed within the target text.
    bos_i = None
    ext_bos_case = "unk-ext"    # BOS case evidence for extended BOS (:-)
    bos_case = "unk"            # BOS case evidence (.!?)
    for tgt_i, tgt_tok in enumerate(tgt_toks):
        debug("add_tgt_bos_markup: tgt_tok:", '{0}'.format(tgt_tok))
        if tgt_tok in eos_punc and tgt_tok not in dashes:
            bos_i = tgt_i
        elif is_cased(tgt_tok) and bos_i is not None:
            # Found a cased token at the start of a target sentence that
            # has not yet had a BOS markup element added to it.
            case = ext_bos_case if tgt_toks[bos_i] in extended_eos_punc else bos_case
            if case in ("lower", "title"): case = "unk-" + case[0]
            verbose("adding bos at line", line_number, "after token:", tgt_toks[bos_i], "index:", bos_i,  "case:", case)
            tgt_toks[bos_i] = '{0} <bos case="{1}"/>'.format(tgt_toks[bos_i], case)
            debug("add_tgt_bos_markup: bos_i:", bos_i, "tgt_toks[bos_i]:", tgt_toks[bos_i])
            bos_i = None
    
    # Add BOS case markup at the beginning of the line.
    if len(tgt_toks) > 0:
       tgt_toks[0] = '<bos case="{0}"/> {1}'.format(bos_case, tgt_toks[0])

    return ' '.join(tgt_toks)

def capitalize(tok):
    """Capitalize a token (word) if it doesn't look like a URL.
    
    tok: token to be capitalized
    returns: capitalized token (or original if a URL)
    """
    if tok.startswith("www.") or tok.startswith("http:") or tok.startswith("https:"):
        return tok          # don't capitalize URLs.
    return capitalize_token(tok)

def bos_markup(grp=True):
    """Return a regular expression to match a <bos case="..."/> element.
    
    grp: boolean indicating whether to retain case value subgroup.
    returns: regular expression text
    """
    return '<bos +case="({0}.+?)"/>'.format("" if grp else "?:")

tok_re = re.compile('(?:{0})|(?:\S+)'.format(bos_markup(False)))
bos_re = re.compile(bos_markup(True))

def remove_bos_markup(line, line_number):
    """Remove beginning-of-sentence XML markup from the tokens of a sentence,
    applying case changes indicated by the markup.
    
    line: line of target language text with BOS markup
    line_number: line number of the sentence (for error messages)
    returns: target language text with BOS markup removed and BOS case applied
    """
    toks = tok_re.findall(line) # reparse necessary
    case = None
    for i, tok in enumerate(toks):
        bos = bos_re.match(tok)
        if bos:                 # found BOS markup
            case = bos.group(1)
            debug("remove_bos_markup: i:", i, "tok:", tok, "case:", case)
            continue
        debug("remove_bos_markup: i:", i, "tok:", tok, "case:", case)
        if not case:            # skip if not at BOS
            continue
        if not is_cased(tok):   # skip uncased tokens
            continue
        # cased token at BOS -> apply case, if applicable
        if tok.islower():
            prev = toks[i-1] if i else ''
            comma = prev == ',' and i > 1   # ignore ',', not standing alone.
            if comma: prev = toks[i-2]
            debug("remove_bos_markup: prev:", prev, "comma:", comma)
            bol = case.endswith("#")
            if bol:
                case = case[:-1]
            if i is 1 and case in ("unk-l", "unk-l?"):  # word added before time or number in tgt
                case = "unk"
            if tok[0].isnumeric():  # skip when token starts with a digit
                debug("remove_bos_markup: match initial digit")
                pass
            elif prev == '%': 
                # skip start with %
                debug("remove_bos_markup: match %")
                pass
            elif prev == '$' and i-comma > 1 and is_number(toks[i-comma-2]): 
                # skip start with 'number $'
                debug("remove_bos_markup: match $")
                pass
            elif case == "title*":      # sentence is preceded by number or time
                debug("remove_bos_markup: match title*")
                toks[i] = capitalize(tok)
            elif bol and not comma and is_section_number(prev) and case in ("unk-t", "unk-t?", "title"):
                debug("remove_bos_markup: match section number")
                toks[i] = capitalize(tok)
            elif is_numeric(prev):
                # skip start with number, time, phone #, or other numeric token
                debug("remove_bos_markup: match numeric")
                pass
            elif case == "title":
                debug("remove_bos_markup: match title")
                toks[i] = capitalize(tok)
            elif case in ("unk-l", "unk-l?"):
                debug("remove_bos_markup: match unk-l")
                pass
            elif (case in ("unk-t", "unk-t?") or case == "unk" and title_unk_case 
                  or case == "unk-ext" and title_unk_ext_case):
                debug("remove_bos_markup: match", case)
                toks[i] = capitalize(tok)
        case = None
    return ' '.join((tok for tok in toks if not bos_re.match(tok)))

def main():
    if cmd_args.tgtfile.name.endswith('.gz'):
        cmd_args.tgtfile.close()
        cmd_args.tgtfile = gzip.open(cmd_args.tgtfile.name, 'rb')
    if cmd_args.outfile.name.endswith('.gz'):
        cmd_args.outfile.close()
        cmd_args.outfile = gzip.open(cmd_args.outfile.name, 'wb')
    if cmd_args.tgtbos and cmd_args.tgtbos.name.endswith('.gz'):
        cmd_args.tgtbos.close()
        cmd_args.tgtbos = gzip.open(cmd_args.tgtbos.name, 'wb')
    
    try:
        codecs.lookup(cmd_args.encoding)
    except LookupError:
        fatal_error("Illegal encoding specified for -enc (--encoding) option: '{0}'".format(cmd_args.encoding))
    
    reader = codecs.getreader(cmd_args.encoding)
    writer = codecs.getwriter(cmd_args.encoding)
    
    tgtfile = reader(cmd_args.tgtfile)
    outfile = writer(cmd_args.outfile)
    tgtbosfile = writer(cmd_args.tgtbos) if cmd_args.tgtbos else None
    sys.stderr = writer(sys.stderr)  # for  utf-8 debug msgs
    
    tgt_line_cnt = 0
    for tgt_line in tgtfile:
        tgt_line_cnt += 1
        debug("tgt line #:", tgt_line_cnt+1)
        tgt_bos_line = add_tgt_bos_markup(tgt_line, tgt_line_cnt)
        if tgtbosfile is not None:
            print(tgt_bos_line, file=tgtbosfile)
        result_line = remove_bos_markup(tgt_bos_line, tgt_line_cnt)
        print(result_line, file=outfile)
    
    outfile.close()
    if tgtbosfile is not None: tgtbosfile.close()
    tgtfile.close()
    
    if cmd_args.time:
        print("boscap.py: boscap.py took", time.time() - start_time, "seconds.", file=sys.stderr)


if __name__ == '__main__':
    main()
