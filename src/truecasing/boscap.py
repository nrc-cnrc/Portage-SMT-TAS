#!/usr/bin/env python
# coding=utf-8

# @file boscap.py
# @brief Apply beginning-of-sentence (BOS) capitalization.
#
# This script takes into account source case information and sentence
# break markers to determine the BOS capitalization points.
#
# This is accomplished by: first adding BOS markup to identify the BOS truecase
# form of each sentence of norm-cased source language text based on
# differences between the original truecased source language text and the
# norm cased source language text; then transferring the BOS markup from
# the norm cased source lanuage text to the truecased target text using
# phrase alignments from PAL file to map between the two; and finally,
# removing the BOS markup from the truecased target language text, applying
# the indicated BOS case as appropriate.
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

printCopyright("boscap.py", 2011);
os.environ['PORTAGE_INTERNAL_CALL'] = '1';


usage = "boscap.py [options] tgtfile orig_srcfile nc1_srcfile palfile [outfile]"
help = """
Add beginning-of-sentence (BOS) capitalization to truecased target language
text (produced by casemark.py) using casing information gleaned from the
source language text and by examining the sentence break markers in the
target language text.
"""

parser = ArgumentParser(usage=usage, description=help, add_help=False)
parser.add_argument("-h", "-help", "--help", action=HelpAction)
parser.add_argument("-v", "--verbose", action=VerboseAction)
parser.add_argument("-d", "--debug", action=DebugAction)
parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                    help="file encoding [%(default)s]")
parser.add_argument("-srcbos", "--srcbos", nargs='?', type=FileType('w'), default=None,
                    help="output intermediate file of source text with BOS markup [%(default)s]")
parser.add_argument("-tgtbos", "--tgtbos", nargs='?', type=FileType('w'), default=None,
                    help="output intermediate file of target text with BOS markup [%(default)s]")
parser.add_argument("-ssp", "--ssp", action='store_true', default=False,
                    help="indicates <orig_srcfile>, <nc1_srcfile> were tokenized "
                         "with -ss -p, but not tgtfile and palfile [%(default)s]")
parser.add_argument("-u", "--unk", choices=("title-basic","title-all","lower"), dest="unk_case", default="title-all",
                    help="casing strategy to apply for unknown BOS case (unk) [%(default)s]")

parser.add_argument("tgtfile", type=FileType('r'), 
                    help="truecased target language text file to capitalize BOS in")
parser.add_argument("orig_srcfile", type=FileType('r'), 
                    help="original truecased source language text file")
parser.add_argument("nc1_srcfile", type=FileType('r'), 
                    help="norm-cased source language text file")
parser.add_argument("palfile", type=FileType('r'),
                    help="phrase alignment file (pal) mapping src phrases to tgt phrases")

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

def bos_markup(grp=True):
    """Return a regular expression to match a <bos case="..."/> element.
    
    grp: boolean indicating whether to retain case value subgroup.
    returns: regular expression text
    """
    return '<bos +case="({0}.+?)"/>'.format("" if grp else "?:")

tok_re = re.compile('(?:{0})|(?:\S+)'.format(bos_markup(False)))

def decide_unknown_case(toks, i):
    """Return case decision for no reference along with updated sentence start index.
    
    toks: list of tokens in the sentence
    i: index of the cased token to decide the case of
    """
    tok = toks[i]
    assert is_cased(tok)
    def _case_evidence(tok):
        if tok.istitle(): return "unk-t"
        if tok.islower(): return "unk-l"
        if tok[first_cased(tok)].isupper(): return "unk-t?" 
        return "unk-l?"
    # cased token - BOS candidate
    prev = toks[i-1] if i else ''
    comma = prev == ',' and i > 1
    if comma: prev = toks[i-2]
    if prev in ('%', '$') and i-comma > 1 and is_number(toks[i-comma-2]):
        return _case_evidence(tok), i-comma-2
    if prev == '%' or is_number(prev) or is_time(prev):
        return _case_evidence(tok), i-comma-1
    if tok.islower():
        return "lower", i
    return _case_evidence(tok), i

def add_bos_markup(nc1_line, orig_line, line_number):
    """Add beginning-of-sentence XML markup to the tokens of a sentence.
    
    <bos case="..."/> elements are preprended to tokens as necessary.
    
    nc1_line: line of norm cased source language text to mark up
    orig_line: corresponding line of original truecased source language text
    line_number: line number of the sentence (for error messages)
    returns: norm cased source language text with BOS markup added
    """
    toks = nc1_line.split()
    orig_toks = orig_line.split()
    start = True
    for i, tok in enumerate(toks):
        debug("add_bos_markup: i", i, "tok:", tok, "orig_tok:", orig_toks[i], "start:", start)
        case = None
        if is_cased(tok):
            if orig_toks[i] != tok:         # original: title case; norm: lowercase
                if orig_toks[i].lower() != tok.lower():
                    fatal_error("Text mismatch at line", line_number)
                prev = toks[i-1] if i else ''
                numbered = '*' if is_number(prev) or is_time(prev) else ''
                bos_case = "title" + numbered
            elif start:
                bos_case, i = decide_unknown_case(toks, i)
            else: continue      # not BOS, so skip processing
            if i is 0 and bos_case in ("unk-t", "unk-t?", "title"):
                bos_case += "#"
            toks[i] = '<bos case="{0}"/> {1}'.format(bos_case, toks[i])
            debug("add_bos_markup: adding bos at i:", i, "toks[i]:", toks[i])
            start = False
        else:
            is_punc = tok in eos_punc
            if is_punc and tok in dashes:
                prev = toks[i-1] if i else ''
                next = toks[i+1] if i+1 < len(toks) else ''
                if not i or is_number(prev) and is_number(next) or is_time(next):
                    is_punc = False
            start = is_punc or start                        # ignore )"           
    return ' '.join(toks)

pal_re = re.compile('(\d+):(\d+)-(\d+):(\d+)-(\d+)')

class Pal(object):
    """Phrase alignment object."""
    class phrase(object):
        def __init__(self, start, end):
            self.start = int(start)
            self.end = int(end)
    
    def __init__(self, src, tgt):
        self.src= self.phrase(src[0],src[1])
        self.tgt= self.phrase(tgt[0],tgt[1])

def move_bos_markup(src_line, tgt_line, pal_line, line_number):
    """Move beginning-of-sentence XML markup for a sentence in the source
    language to the corresponding target language text.
    
    src_line: line of norm cased source language text with BOS markup
    tgt_line: corresponding line of target language text to mark up
    pal_line: line of phrase alignments mapping between source and target phrases
    line_number: line number of the sentence (for error messages)
    returns: target language text with BOS markup added
    """
    src_toks = tok_re.findall(src_line)
    tgt_toks = tgt_line.split()
    #Parse the phrase alignment record
    pal = []
    for m in pal_re.finditer(pal_line):
        pal.append(Pal((m.group(2), m.group(3)), (m.group(4), m.group(5))))
    pal.sort(key=lambda x:x.src.start)
    
    # First, transfer the markup from the source tokens to the corresponding 
    # target tokens according to the phrase alignment information.
    src_i = 0   # bos markup tokens are not enumerated in src_i
    pal_i = 0
    tgt_i = 0
    prev_tgt_i = None
    prev_src_tok = None
    for src_tok in src_toks:
        bos = bos_re.match(src_tok)
        debug("move_bos_markup: src_tok:", '{0}'.format(src_tok), "src_i:", src_i, "bos:", bos is not None)
        if not bos:     # normal token
            while pal_i < len(pal):
                if src_i > pal[pal_i].src.end:
                    pal_i += 1
                    prev_tgt_i = None   # phrase change so no prev_tgt_in in the phrase
                else: break
            else:
                fatal_error("# src tokens mismatch with pal info at line", line_number)
            debug("move_bos_markup: pal_i:", pal_i, "pal:", pal[pal_i].src.start, pal[pal_i].src.end, 
                  pal[pal_i].tgt.start, pal[pal_i].tgt.end)
            if src_tok in eos_punc:
                # Find next EOS punctuation in the target phrase.
                for i in range(max(pal[pal_i].tgt.start, prev_tgt_i), pal[pal_i].tgt.end+1):
                    if tgt_toks[i] in eos_punc:
                        tgt_i = i + 1
                        break
                else:   # no more EOS punctuation in this target token
                    tgt_i = prev_tgt_i
            src_i += 1
            prev_src_tok = src_tok
        else:           # BOS markup
            if tgt_i == -1:   # no src punc., so treat prev token as punc.
                for i in range(pal[pal_i].tgt.end, max(pal[pal_i].tgt.start, prev_tgt_i)-1, -1):
                    if tok_re.findall(tgt_toks[i])[-1] == prev_src_tok:
                        tgt_i = i + 1
                        break
                else:
                    tgt_i = pal[pal_i].tgt.start + (src_i - pal[pal_i].src.start) + 1
            if tgt_i is not None and tgt_i < len(tgt_toks):
                tgt_toks[tgt_i] = "{0} {1}".format(bos.group(), tgt_toks[tgt_i])
                debug("move_bos_markup: tgt_i:", tgt_i, "tgt_toks[tgt_i]:", tgt_toks[tgt_i])
                prev_tgt_i = tgt_i
            tgt_i = -1

    if src_i != pal[-1].src.end + 1:
        fatal_error("# src tokens mismatch with pal info at line", line_number)
    if len(tgt_toks) != max(pal, key=lambda x:x.tgt.end).tgt.end + 1:
        fatal_error("# tgt tokens mismatch with pal info at line", line_number)

    # Add BOS case markup in target for sentences that did not have markup
    # transferred from the source, e.g. when the target has more sentences
    # than the source.
    bos_i = None
    ext_bos_case = "unk-ext"    # BOS case evidence for extended BOS (:-)
    bos_case = "unk"            # BOS case evidence (.!?)
    for tgt_i, tgt_tok in enumerate(tgt_toks):
        bos = bos_re.match(tgt_tok)
        debug("move_bos_markup: tgt_tok:", '{0}'.format(tgt_tok), "bos:", bos is not None)
        if bos:
            tgt_tok = tgt_tok[bos.end()+1:]
            # Record most recent BOS case evidence
            if bos_i is not None and tgt_toks[bos_i] in extended_eos_punc:
                ext_bos_case = bos.group(1)
            else:
                bos_case = bos.group(1)
            bos_i = None
        if tgt_tok in eos_punc and tgt_tok not in dashes:
            bos_i = tgt_i
        elif is_cased(tgt_tok) and bos_i is not None:
            # Found a cased token at the start of a target sentence that
            # has not yet had a BOS markup element added to it.
            case = ext_bos_case if tgt_toks[bos_i] in extended_eos_punc else bos_case
            if case in ("lower", "title"): case = "unk-" + case[0]
            verbose("adding bos at line", line_number, "after token:", tgt_toks[bos_i], "index:", bos_i,  "case:", case)
            tgt_toks[bos_i] = '{0} <bos case="{1}"/>'.format(tgt_toks[bos_i], case)
            debug("move_bos_markup: bos_i:", bos_i, "tgt_toks[bos_i]:", tgt_toks[bos_i])
            bos_i = None

    return ' '.join(tgt_toks)

def capitalize(tok):
    """Capitalize a token (word) if it doesn't look like a URL.
    
    tok: token to be capitalized
    returns: capitalized token (or original if a URL)
    """
    if tok.startswith("www.") or tok.startswith("http:"):
        return tok          # don't capitalize URLs.
    return capitalize_token(tok)

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
            if prev == '%': 
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
    if cmd_args.orig_srcfile.name.endswith('.gz'):
        cmd_args.orig_srcfile.close()
        cmd_args.orig_srcfile = gzip.open(cmd_args.orig_srcfile.name, 'rb')
    if cmd_args.nc1_srcfile.name.endswith('.gz'):
        cmd_args.nc1_srcfile.close()
        cmd_args.nc1_srcfile = gzip.open(cmd_args.nc1_srcfile.name, 'rb')
    if cmd_args.palfile.name.endswith('.gz'):
        cmd_args.palfile.close()
        cmd_args.palfile = gzip.open(cmd_args.palfile.name, 'rb')
    if cmd_args.outfile.name.endswith('.gz'):
        cmd_args.outfile.close()
        cmd_args.outfile = gzip.open(cmd_args.outfile.name, 'wb')
    if cmd_args.srcbos and cmd_args.srcbos.name.endswith('.gz'):
        cmd_args.srcbos.close()
        cmd_args.srcbos = gzip.open(cmd_args.srcbos.name, 'wb')
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
    orig_srcfile = reader(cmd_args.orig_srcfile)
    nc1_srcfile = reader(cmd_args.nc1_srcfile)
    palfile = reader(cmd_args.palfile)
    outfile = writer(cmd_args.outfile)
    srcbosfile = writer(cmd_args.srcbos) if cmd_args.srcbos else None
    tgtbosfile = writer(cmd_args.tgtbos) if cmd_args.tgtbos else None
    sys.stderr = writer(sys.stderr)  # for  utf-8 debug msgs
    
    src_line_cnt = 0;
    tgt_line_cnt = 0;
    src_bos_lines = []
    for nc1_line in nc1_srcfile:
        src_line_cnt += 1
        debug("***src line #:", src_line_cnt, "tgt line #:", tgt_line_cnt+1)
        orig_line = orig_srcfile.readline()
        src_bos_lines.append(add_bos_markup(nc1_line, orig_line, src_line_cnt))
        if not cmd_args.ssp or nc1_line == '\n':
            src_bos_line = ' '.join(src_bos_lines)
            if srcbosfile is not None:
                print(src_bos_line, file=srcbosfile)
            tgt_line_cnt += 1
            tgt_line = tgtfile.readline()
            pal_line = palfile.readline()
            tgt_bos_line = move_bos_markup(src_bos_line, tgt_line, pal_line, tgt_line_cnt)
            if tgtbosfile is not None:
                print(tgt_bos_line, file=tgtbosfile)
            result_line = remove_bos_markup(tgt_bos_line, tgt_line_cnt)
            print(result_line, file=outfile)
            src_bos_lines = []
    
    nc1_srcfile.close()
    outfile.close()
    if srcbosfile is not None: srcbosfile.close()
    if tgtbosfile is not None: tgtbosfile.close()
    if orig_srcfile.readline(1) != '':
        fatal_error("More lines in <nc1_srcfile> than <orig_srcfile> at line", src_line_cnt)
    orig_srcfile.close()
    if tgtfile.readline(1) != '':
        fatal_error("More lines in <tgtfile> than merged <nc1_srcfile> at line", tgt_line_cnt)
    tgtfile.close()
    if palfile.readline(1) != '':
        fatal_error("More lines in <palfile> file than merged <nc1_srcfile> at line", tgt_line_cnt)
    palfile.close()


if __name__ == '__main__':
    main()
