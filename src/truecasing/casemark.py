#!/usr/bin/env python
# coding=utf-8

# @file casemark.py
# @brief Add/remove case XML markup (srccase elements).
#
# NOTE: casemark.py should be called with the following environment setting: 
#   LD_PRELOAD=libstdc++.so
# to cause the correct initialization of C++ exceptions and the C++ standard 
# stream objects when the pylm shared library is loaded.
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
from ctypes import *
import os
import os.path
import gzip
import math

# If this script is run from within src/ rather than from the installed bin
# directory, we add src/utils to the Python module include path (sys.path).
if sys.argv[0] not in ('', '-c'):
    bin_path = os.path.dirname(sys.argv[0])
    if os.path.basename(bin_path) != "bin":
        sys.path.insert(1, os.path.normpath(os.path.join(bin_path, "..", "utils")))

from portage_utils import *

printCopyright("casemark.py", 2011);
os.environ['PORTAGE_INTERNAL_CALL'] = '1';


usage = "casemark.py [options] [-a|-r] [infile [outfile]]"
help = """
Either add (-a) or remove (-r) case markup information to/from <infile>.
The add operation adds markup to identify the truecase form of each token in
<infile>, and lowercases the normal text. The remove operation removes the case
markup replacing each marked up text string by the appropriate truecase form. 
Default is -a.

<infile> is expected to be sentence split and tokenized.

The point of this program is to preserve approriate case through translation.
"""

parser = ArgumentParser(usage=usage, description=help, add_help=False)
parser.add_argument("-h", "-help", "--help", action=HelpAction)
parser.add_argument("-v", "--verbose", action=VerboseAction)
parser.add_argument("-d", "--debug", action=DebugAction)
parser.add_argument("-enc", "--encoding", nargs='?', default="utf-8",
                    help="file encoding [%(default)s]")
markup_group = parser.add_mutually_exclusive_group()
markup_group.add_argument("-a", "--add-markup", dest="add_markup", 
                          action="store_true", default=True,
                          help="add case markup to source text [do]")
markup_group.add_argument("-r", "--remove-markup", dest="add_markup", 
                          action="store_false",
                          help="remove case markup from target text, applying case rules [don't]")
parser.add_argument("-lm", "--lm", nargs='?', default=None,
                    help="for -a, source nc1 lm file for identifying acronyms [%(default)s]")
parser.add_argument("-u", "--upper", type=int, choices=(1,2,3), default=2,
                    help="for -r, apply upper case rule [%(default)s]")
parser.add_argument("-t", "--title", type=int, choices=(0,1,2), default=0,
                    help="for -r, apply title case rule [%(default)s]")
parser.add_argument("infile", nargs='?', type=FileType('r'), default=sys.stdin, 
                    help="input file of text to add/remove case markup to")
parser.add_argument("outfile", nargs='?', type=FileType('w'), default=sys.stdout, 
                    help="output text file with case markup added/removed")

cmd_args = parser.parse_args()


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

def is_cased(s):
    """Return True if string s contains some cased characters; otherwise false."""
    return s.lower() != s.upper()

def is_title(s):
    """Return True if a single token string s is title cased.
    
    is_title(s) treats strings containing hyphens and/or slashes differently
    than s.istitle() does:
    is_title("Hyphened-word") returns True; "Hyphened-word".istitle() returns False
    is_title("Hyphened-Word") returns False; "Hyphened-Word".istitle() returns True
    
    s: single token string to be checked for title case
    returns: True is s is title cased or False if s is not title cased.
    """
    for idx, c in enumerate(s):
        if c.islower():
            return False
        if c.isupper():
            for c2 in s[idx+1:]:
                if c2.isupper():
                    return False
            return True
    return False

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

esc_2_char = {'&quote;': '"', '&apos;': "'", '&amp;': '&', '&lt;': '<', '&gt;': '>'}
char_2_esc = {esc_2_char[k]: k for k in esc_2_char}
xml_unesc_re = re.compile('|'.join(esc_2_char.keys()))

def escape(s, c):
    """Returns string s with occurrences of c replaced by its escape sequence ('&xyz;')"""
    return re.sub(c, char_2_esc[c], s) if c in char_2_esc else s

def escape_lt_gt_amp(s):
    """Returns string with occurrences of '<' replaced by '&lt;', '>' by '&gt;' and '&' by '&amp;'."""
    return escape(escape(escape(s, '&'), '<'), '>');

def escape_quote(s):
    """Returns string with occurrences of '"' replaced by '&quote;'. """
    return escape(s, '"')

def unescape(s, c):
    """Returns string s with occurrences the escape sequence for c ('&xyz;') replaced by c"""
    return re.sub(char_2_esc[c], c, s) if c in char_2_esc else s

def unescape_all(s):
    """Returns string with all XML character escapes replaced by the characters represented."""
    return xml_unesc_re.sub(lambda m: esc_2_char[m.group()], s)

dashes = ('-', '–', '—')
extended_eos_punc = ((':',) + dashes)
eos_punc = ('.', '!', '?') + extended_eos_punc
grp_punc = (",", "&", "/", "#") + dashes    # punctuation allowed internal to a case grouping

def determine_case(toks, idx):
    """Determine the case of a token.
    
    toks: list of tokens in the src line
    idx: index in toks of the token being considered
    returns: "title", "upper", or None (for lower case)
    """
    tok = toks[idx]
    debug("determine_case: idx:", idx, "tok:", tok)
    afu = first_upper(tok) + 1
    if not afu:             # skip lowercase and non-cased tokens
        return None
    afl = first_lower(tok) + 1
    if afl:                 # contains some lowercase
        return "title" if afu < afl and first_upper(tok[afu:]) is -1 else "mixed"
    if first_upper(tok[afu:]) is not -1:    # uppercase > length 1
        return "upper"
    
    # single uppercase: examine context
    if tok[0] == "'" and tok[-1] != "'":    # e.g. 'S 'T
        return "upper"
    if len(tok) is 3 and tok[0].isdigit() and tok[2].isdigit():    # i.e. a postal code
        return "upper"

    def check_case_nearby(t):
        if first_lower(t) is not -1:    # mixed/lower case evidence
            return "title"
        tafu = first_upper(t) + 1
        if tafu and first_upper(t[tafu:]) is not -1:  # uppercase > length 1
            return "upper"
        if tafu:
            return "both"
        return None

    # look at subsequent tokens if a single uppercase character
    case_fwd = None
    for t in toks[idx+1:]:
        case_fwd = check_case_nearby(t)
        if case_fwd == "both": continue
        if case_fwd or t not in grp_punc: break
    if case_fwd == "both": case_fwd = None
    debug("determine_case: case_fwd:", case_fwd)
    if case_fwd:
        return case_fwd
    return "upper"

def add_log_prob(a, b):
    if math.isinf(a) and a < 0:
        return b
    if math.isinf(b) and b < 0:
        return a
    return math.log10(math.pow(10., a) + math.pow(10., b))

def is_acronym(toks, idx):
    """Determine if an uppercase token is an acronym.
    
    toks: list of tokens in the src line
    idx: index in toks of the token being considered
    returns: True if the token is an acronym; False if not.
    """
    tok = toks[idx]
    if cmd_args.lm is None:
        return len(tok) < 1    # set to 3 or 4 to enable heuristic
    if not tok.isupper():
        debug("is_acronym returning False for idx", idx, "tok", tok) 
        return False
    tok_lower = tok.lower()
    tok_cap = capitalize_token(tok_lower)
    lm_order = pylm.get_lm_order()
    pycontext = toks[idx+1:idx+lm_order]
    uc_prob = pylm.get_word_prob(tok, pycontext)
    nc_prob = pylm.get_word_prob(tok_lower, pycontext)
    if tok_cap != tok:
        prob = pylm.get_word_prob(tok_cap, pycontext)
        nc_prob = add_log_prob(nc_prob, prob)
    if any(t.isupper() for t in pycontext): # need to consider lowercase contexts ?
        context_prob = pylm.get_context_prob(pycontext)
        uc_prob += context_prob     # weight by context probability
        nc_prob += context_prob     # weight by context probability
        new_pycontext = [w for w in pycontext]
        for i, t in enumerate(new_pycontext):
            if t.isupper():
                new_pycontext[i] = t.lower()
            elif t in eos_punc and t not in dashes:
                break
        if new_pycontext != pycontext:
            context_prob = pylm.get_context_prob(new_pycontext)
            prob = pylm.get_word_prob(tok, new_pycontext)
            prob += context_prob     # weight by context probability
            uc_prob = add_log_prob(uc_prob, prob)
            prob = pylm.get_word_prob(tok_lower, new_pycontext)
            prob += context_prob     # weight by context probability
            nc_prob = add_log_prob(nc_prob, prob)
            if tok_cap != tok:
                prob = pylm.get_word_prob(tok_cap, new_pycontext)
                prob += context_prob     # weight by context probability
                nc_prob = add_log_prob(nc_prob, prob)
    debug("is_acronym returning", uc_prob > nc_prob, "for idx", idx, "tok", tok, 
          ";", uc_prob, ">" if uc_prob > nc_prob else "<=", nc_prob)
    return uc_prob > nc_prob

def count_case_following(case, toks, idx):
    """Count the occurrences of "upper" or "title" case following a token.
    
    case: case to match: either "upper" or "title"
    toks: list of tokens in the src line
    idx: index in toks of the token in being considered
    returns: 2-tuple of a count of occurrences of the matched case following
        the token and a boolean indicating whether the token is an acronym
    """
    def match_case_nearby(t, case):
        if case == "upper":
            return t.isupper()
        elif case == "title":
            return is_title(t)
        return False

    debug("count_case_following: case:", case, "idx:", idx, "tok:", toks[idx])
    is_anym = case == "upper" and is_acronym(toks, idx)
    check_acronyms = is_anym or case == "upper" and toks[idx] in grp_punc
    
    # look at subsequent tokens
    cnt_fwd = 0
    cnt_punc = 0
    cnt_anym_fwd = 0
    for i, t in enumerate(toks[idx+1:], idx+1):
        if match_case_nearby(t, case):
            if check_acronyms and cnt_anym_fwd == cnt_fwd:
                if is_acronym(toks, i):
                    cnt_anym_fwd += 1 + cnt_punc
            cnt_fwd += 1 + cnt_punc
            cnt_punc = 0
        elif t in grp_punc:
            cnt_punc += 1
        else: break
    if cnt_anym_fwd == cnt_fwd:
        cnt_fwd = 0
    debug("count_case_following returning", cnt_fwd, is_anym)
    return cnt_fwd, is_anym

def add_markup(toks, idx):
    """Add srccase XML markup to a token.
    
    toks: list of tokens in the src line
    idx: index in toks of the token for which case is to be marked up
    returns: 2-tuple of <srccase ...>...</srccase> XML string (or unmarked
        token string) along with a count of the number of tokens processed
        (1 for an unmarked token)
    """
    tok = toks[idx]
    debug("add_markup: idx:", idx, "toks[idx]: '{0}'".format(tok))
    tok_lower = tok.lower()
    tok_lower_esc = escape_lt_gt_amp(tok_lower)
    if tok_lower == tok:    # skip lowercase and non-cased tokens
        return tok_lower_esc, 1
    case = determine_case(toks, idx)
    case_cnt, is_anym = count_case_following(case, toks, idx)
    if case == "upper" and case_cnt is 0 and cmd_args.lm is not None:
        verbose("add_markup:", "not marking" if is_anym else "MARKING",
                "line", line_number, ": '{0}'".format(tok))
    if is_anym and case_cnt is 0:
        return tok_lower_esc, 1
    sc_toks = escape_lt_gt_amp(' '.join(toks[idx:idx+case_cnt+1]))
    return ('<srccase case="{0}" src="{1}">{2}</srccase>'.
            format(case, escape_quote(sc_toks), sc_toks.lower()), case_cnt+1)

def add_markup_line(line):
    """Add srccase XML markup to a line of text.
    
    line: text string to be marked up
    returns: marked up text string
    """
    debug("add_markup_line:", line_number)
    toks = line.split()
    idx = 0
    results = []
    while idx < len(toks):
        result, cnt = add_markup(toks, idx)
        results.append(result)
        idx += cnt
    return " ".join(results)

def attr(name, grp=True):
    """Return a regular expression to match an XML attribute.
    
    name: name of the attribute to match
    grp: boolean indicating whether to retain the attribute value subgroup.
    returns: regular expression text
    """
    return '(?: +{0}="({1}[^"]+)")'.format(name, "" if grp else "?:")

srccase_re = re.compile('<srccase{0}{1}> *((?:(?!<srccase).)+?) *</srccase>'.
                        format(attr("case"), attr("src")))

def oov_src(grp):
    """Return a regular expression to match an <OOV src="...">...</OOV> element.
    
    grp: boolean indicating whether to retain src value and oov text subgroups.
    returns: regular expression text
    """
    return '<OOV{0}?>({1}.+?)</OOV>'.format(attr("src", grp), "" if grp else "?:")

oov_re = re.compile(oov_src(True))
rm_toks_re = re.compile('(?:{0})|(?:\S+)'.format(oov_src(False)))

def apply_case(case, tgt, cased_src):
    """Apply the specified casing to the target text using cased_src as a
    reference for resolving the case of OOVs.
    
    case: case to apply, which can be "upper", "lower", or None
    tgt: target text
    cased_src: corresponding true-cased source text
    returns: cased target text
    """
    debug("apply_case: case:", case, "tgt:", tgt, "cased_src:", cased_src)
    tgt_toks = rm_toks_re.findall(tgt)
    if cased_src is None:
        cased_src = tgt
    cased_src_toks = rm_toks_re.findall(cased_src)
    for i,tok in enumerate(tgt_toks):
        oov_match = oov_re.match(tok)
        if oov_match:
            # OOVs will be expanded later.
            if not oov_match.group(1):  # add src attribute to OOV if needed
                # Find the corresponding source token.
                tgt_tok = oov_match.group(2)
                look_fwd = i < len(tgt_toks)/2
                for esc_src_tok in cased_src_toks if look_fwd else reversed(cased_src_toks):
                    src_tok = unescape_all(esc_src_tok)
                    if src_tok.lower() == tgt_tok:
                        # For later OOV expansion
                        tgt_toks[i] = '<OOV src="{0}">{1}</OOV>'.format(esc_src_tok, tgt_tok)
                        break
        elif case == "upper":
            tgt_toks[i] = tok.upper()
        elif case == "title":
            tgt_toks[i] = capitalize_token(tok)
    debug("apply_case returning:", *tgt_toks)
    return ' '.join(tgt_toks)

def rm_markup(match):
    """Remove the XML markup from a matched srccase element, applying the case
    change rules to the target text in the element.
    
    match: match object of the element from which srccase markup is to be stripped
    returns: cased target text with markup removed
    """
    case = match.group(1)
    cased_src = match.group(2)
    tgt = match.group(3)        # may or may not carry case already
    debug("rm_markup: '{0}'".format(match.group()))
    if cased_src is None:       # no markup in token - return as is
        error("rm_markup got a token without markup: '{0}'".format(match.group()))
        return match.group()
    debug("rm_markup: case:", case, "src:", cased_src, "tgt:", tgt)
    apply = None
    if case == "upper":    
        # Apply uppercase according to upper rule:
        # 1: src is multi-word UC or is a single UC word sentence
        # 2: src is multi-word UC or is the first cased word in the sentence
        # 3: src is uppercase
        if cmd_args.upper is 3 or ' ' in cased_src:
            apply = "upper"
        elif cmd_args.upper is 2:
            if not is_cased(match.string[:match.start()]):
                apply = "upper"
            elif rm_toks_re.findall(match.string, 0, match.start())[-1] in eos_punc:
                apply = "upper"
        elif not is_cased(match.string[:match.start()]) and \
             not is_cased(match.string[match.end():]):
            apply = "upper"
    elif cmd_args.title and case == "title" and tgt.islower():
        if cmd_args.title is 2 or ' ' in cased_src:
            apply = "title"
    return apply_case(apply, tgt, cased_src)

def rm_markup_line(line):
    """Remove srccase and OOV XML markup from a line of text.
    
    line: text string to remove markup from
    returns: text string with markup removed and case rules applied
    """
    debug("rm_markup_line:", line_number)
    n = 1
    while n:
        line, n = srccase_re.subn(rm_markup, line)
        if n: debug("rm_markup_line:", line)
    line = oov_re.sub(lambda m: m.group(1) if m.group(1) else m.group(2), line)
    return unescape_all(line)

def main():
    global pylm, line_number
    
    if cmd_args.infile.name.endswith('.gz'):
        cmd_args.infile.close()
        cmd_args.infile = gzip.open(cmd_args.infile.name, 'rb')
    
    if cmd_args.outfile.name.endswith('.gz'):
        cmd_args.outfile.close()
        cmd_args.outfile = gzip.open(cmd_args.outfile.name, 'wb')
    
    try:
        codecs.lookup(cmd_args.encoding)
    except LookupError:
        fatal_error("Illegal encoding specified for -enc option:",
                    "'{0}'".format(cmd_args.encoding))
    
    infile = codecs.getreader(cmd_args.encoding)(cmd_args.infile)
    outfile = codecs.getwriter(cmd_args.encoding)(cmd_args.outfile)
    sys.stderr = codecs.getwriter(cmd_args.encoding)(sys.stderr)  # for  utf-8 debug msgs
    
    if cmd_args.lm is not None and cmd_args.add_markup:
        # Make sure the infile is an actual seekable file because we need to read it twice.
        try:
            infile.seek(0)
        except (IOError):
            fatal_error("<infile> must be a real file (not stdin or pipe) when using -lm option.")
        pylm = PyLM(cmd_args.encoding)
        pylm.load_vocab(infile)
        verbose("Vocab filter size:", pylm.get_vocab_filter_size())
        verbose("Loading LM...")
        pylm.load_lm(cmd_args.lm)
        
    process_line = add_markup_line if cmd_args.add_markup else rm_markup_line
    line_number = 0    
    
    for line in infile:
        line_number += 1
        line = process_line(line.rstrip('\n'))
        print(line, file=outfile,)
        
    infile.close()
    outfile.close()


class PyLM(object):
    """Wrapper for accessing Portage LM C++ class."""
    def __init__(self, encoding="utf-8"):
        self.__vf = None
        self.__lm = None
        self.__lm_order = None
        self.set_encoding(encoding)
        self.__lib_pylm = self.__load_lib_pylm()
        
    def __load_lib_pylm(self):
        """Load the dynamic C/C++ library for accessing the LM."""
        lib_pylm = cdll.LoadLibrary('libportage_truecasing.so')
        if lib_pylm is None:
            raise IOError("libportage_truecasing.so failed to load.")
        lib_pylm.VocabFilter_new.argtypes = []
        lib_pylm.VocabFilter_new.restype = POINTER(c_void_p)
        lib_pylm.VocabFilter_delete.argtypes = [POINTER(c_void_p)]
        lib_pylm.VocabFilter_delete.restype = None
        lib_pylm.VocabFilter_add.argtypes = [POINTER(c_void_p), c_char_p]
        lib_pylm.VocabFilter_add.restype = c_uint
        lib_pylm.VocabFilter_add_n.argtypes = [POINTER(c_void_p), c_uint, POINTER(c_char_p)]
        lib_pylm.VocabFilter_add_n.restype = None
        lib_pylm.VocabFilter_size.argtypes = [POINTER(c_void_p)]
        lib_pylm.VocabFilter_size.restype = c_uint
        lib_pylm.VocabFilter_index.argtypes = [POINTER(c_void_p), c_char_p]
        lib_pylm.VocabFilter_index.restype = c_uint
        lib_pylm.PLM_create.argtypes = [c_char_p, POINTER(c_void_p)]
        lib_pylm.PLM_create.restype = POINTER(c_void_p)
        lib_pylm.PLM_getOrder.argtypes = [POINTER(c_void_p)]
        lib_pylm.PLM_getOrder.restype = c_uint
        lib_pylm.PLM_wordProb.argtypes = [POINTER(c_void_p), POINTER(c_void_p), c_char_p, POINTER(c_char_p), c_uint]
        lib_pylm.PLM_wordProb.restype = c_float
        return lib_pylm
    
    def set_encoding(self, encoding):
        """Set the character encoding."""
        self.__encoding = encoding

    def __encode(self, uni_str):
        """Encode a unicode string using the specified encoding."""
        return bytes(bytearray(uni_str, self.__encoding))

    def load_vocab(self, infile):
        """Load the vocab from infile as the vocab filter for the language model.
        
        infile: open input file from which the vocab is loaded; must be seekable.
        returns: the vocab filter.
        """
        self.__vf = self.__lib_pylm.VocabFilter_new()
        for line in infile:
            for tok in line.split():
                tok_lower = tok.lower()
                tok_cap = capitalize_token(tok.lower())
                self.__lib_pylm.VocabFilter_add(self.__vf, self.__encode(tok))
                if tok != tok_lower:
                    self.__lib_pylm.VocabFilter_add(self.__vf, self.__encode(tok_lower))
                if tok != tok_cap:
                    self.__lib_pylm.VocabFilter_add(self.__vf, self.__encode(tok_cap))
        infile.seek(0)
        return self.__vf
    
    def get_vocab_filter(self):
        """Return the vocab filter."""
        return self.__vf
    
    def get_vocab_filter_size(self):
        """Return the vocab filter size."""
        if self.__vf is None:
            raise IOError("Vocab filter not loaded.")
        return self.__lib_pylm.VocabFilter_size(self.__vf)
    
    def load_lm(self, lm_filename, infile=None):
        """Load a language model.
        
        lm_filename: name of the language model file.
        infile: open input file from which the vocab is loaded; must be seekable.
        returns: LM object
        """
        if infile is not None:
            self.__vf = self.load_vocab(infile)
        elif self.__vf is None:
            raise IOError("Vocab filter not loaded.")
        self.__lm = self.__lib_pylm.PLM_create(lm_filename, self.__vf)
        if self.__lm is None:
            raise IOError("LM failed to load.")
        return self.__lm
    
    def get_lm(self):
        """Return the LM."""
        return self.__lm
    
    def get_lm_order(self):
        """Return the language model order"""
        if self.__lm is None:
            raise IOError("LM not loaded.")
        if self.__lm_order is None:
            self.__lm_order = self.__lib_pylm.PLM_getOrder(self.__lm)
        return self.__lm_order
    
    def get_word_prob(self, word, context):
        """Return the word probability in the LM for a word in a given context.
        
        word: word to look up
        context: context of the word, which is a list of length lm_order-1 of words
        returns: log probability for the word given the context
        """
        if self.__lm is None:
            raise IOError("LM not loaded.")
        c_context_type = c_char_p * len(context)
        c_context = c_context_type(*[self.__encode(w) for w in context])
        c_word = self.__encode(word)
        return self.__lib_pylm.PLM_wordProb(self.__lm, self.__vf, c_word,
                                            c_context, len(c_context))

    def get_context_prob(self, context):
        """Return the probability for a context.
        P(c) = P(c1,c2) = P(c1|c2) * P(c2)
        
        context: context list of words in the context
        returns: log probability for the context
        """
        log_prob = 0.
        for i in range(len(context)):
            p = self.get_word_prob(context[i], context[i+1:])
            log_prob += p   # log probabilities, so add log-probs to multiply probs
        return log_prob


if __name__ == '__main__':
    main()
