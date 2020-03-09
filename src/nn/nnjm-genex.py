#!/usr/bin/env python
# -*- coding: utf-8 -*-

# @file nnjm-genex.py
# @brief Generate NNJM training examples.
#
# @author George Foster
#
# Traitement multilingue de textes / Multilingual Text Processing
# Centre de recherche en technologies numériques / Digital Technologies Research Centre
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2014, Sa Majeste la Reine du Chef du Canada /
# Copyright 2014, Her Majesty in Right of Canada

# TODO: check utf8 handling

import sys
import argparse
import os.path
import gzip
from collections import Counter

help = """
Generate examples for training a BBN neural-net joint model from a word-aligned
parallel corpus. For each target token t in the corpus, write to stdout an
example of the form: "sw / h / t", where sw is a window of source words centred
on the word aligned to t, and h is the ngram preceding t. All words are written
as integer indexes. Contexts will be padded as necessary to maintain a fixed
size for all examples.

There are two modes: if the -r switch is set, external vocabularies for
indexing words are read in and fixed; otherwise, vocabularies are acquired from
examples and written when finished. Vocabulary filenames in both cases are
specified by the -voc switch. 

Separate vocabularies are maintained for source words, target context words,
and target output words.  Each vocabulary maps unk, bos, and eos events to 0,
1, and 2.  Automatically-acquired vocabularies map the most frequent n words to
successive indexes above 2, where n is given by the -isv/itv/ov switches. Other
words are replaced by their tags, or by unk if tagged files aren't specified or
if -r is given and the tag is not found in the vocabulary. 

The vocabulary file format is:
   0 <ELID>
   1 <UNK>
   2 <BOS>
   3 <EOS>
   4 word1
   5 word2
   ... 
   n <TAG>:tag1
   n+1 <TAG>:tag2
   ...
That is, special events are coded first (and are optional), words are coded as
themselves, and tags are prefixed with '<TAG>:'. Indexes are not required to be
sequential or contiguous.
"""

parser = argparse.ArgumentParser(
   formatter_class=argparse.RawDescriptionHelpFormatter,
   description=help)
parser.add_argument('-v', action='count', help='verbose output (repeat for more)')
parser.add_argument('-r', action='store_true', help="read in and fix vocabularies [acquire from corpus]")
parser.add_argument('-rev', action='store_true', help='alignment is reversed; contains t-s position pairs')
parser.add_argument('-eos', action='store_true', help='model end of sentence event')
parser.add_argument('-ng', type=int, default=4, help='ngram order (includes current word) [4]')
parser.add_argument('-ws', type=int, default=11, help='source window size [11]')
parser.add_argument('-isv', type=int, default=16000, help='input source voc size (ignored if -r) [16000]')
parser.add_argument('-itv', type=int, default=16000, help='input target voc size (ignored if -r) [16000]')
parser.add_argument('-ov', type=int, default=32000, help='output voc size (ignored if -r) [32000]')
parser.add_argument('-voc', type=str, default="voc", 
   help='base name of vocabulary files [voc] - corresponds to files voc.src, voc.tgt, and voc.out')
parser.add_argument('-stag', type=str, help='source tag file [None]')
parser.add_argument('-ttag', type=str, help='target tag file [None]')
parser.add_argument('src', type=str, help='tokenized source file')
parser.add_argument('tgt', type=str, help='tokenized target file')
parser.add_argument('al', type=str, help='alignment file in sri format')
args = parser.parse_args()

def log(msg):
   if (args.v):
      print >> sys.stderr, msg

def error(msg): 
   print >> sys.stderr, "Error: " + msg
   sys.exit(1)

def count_words(fname):
   """Count word occurrences in a text file."""
   counts = Counter()
   for line in opener(fname, 'r'):
      for tok in line.split():
         counts[tok] += 1
   return counts

def get_index(voc, i, toks, tags, unk, beg):
   """Get index for toks[i] in voc, with backoff to tags[i] or unk."""
   if toks[i] in voc:
      return voc[toks[i]]
   elif tags:
      tag = '<TAG>:' + tags[i]
      if tag in voc:
         return voc[tag]
      elif not args.r:  # add tag to voc
         ind = len(voc) + beg
         voc[tag] = ind
         return ind
      else:
         return unk
   else:
      return unk

def opener(filename, mode):
   return gzip.open(filename, mode) if filename.endswith(".gz") else open(filename, mode)

if args.ws % 2 == 0:
   log("source window size must be an odd number")
   sys.exit(1)

log("Parameter settings:")
for a in sorted(vars(args).keys()):
   log('   ' + a + '=' + str(vars(args)[a]))

elid, unk, bos, eos, beg = 0, 1, 2, 3, 4 # special indexes
special_names = '<ELID>', '<UNK>', '<BOS>', '<EOS>'
vocs = svoc, tvoc, ovoc = {}, {}, {}
voc_files = svoc_file, tvoc_file, ovoc_file = [args.voc + x for x in ('.src', '.tgt', '.out')]
if args.r:
   log("reading vocabulary files")
   for n,v in zip(voc_files, vocs):
      with file(n, 'r') as f:
         for line in f:
            toks = line.split()
            ind, word = int(toks[0]), toks[1]
            if ind < beg:  # leave special names implicit
               if word != special_names[ind]:
                  error("voc file %s: index %d is reserved for %s" % \
                        (n, ind, special_names[ind]))
            else:
               v[word] = ind
else:
   log("defining vocabularies")
   for v in voc_files:
      if os.path.exists(v):
         error("voc file %s exists already, won't overwrite (did you forget -r?)" % v)
   src_counts = count_words(args.src)
   tgt_counts = count_words(args.tgt)
   for i,e in enumerate(src_counts.most_common(args.isv)): svoc[e[0]] = i + beg
   for i,e in enumerate(tgt_counts.most_common(args.itv)): tvoc[e[0]] = i + beg
   for i,e in enumerate(tgt_counts.most_common(args.ov)): ovoc[e[0]] = i + beg

log("indexing and generating examples")

lpad = [str(bos) for i in range(args.ws/2)]
rpad = [str(eos) for i in range(args.ws/2)]
ngpad = [str(bos) for i in range(args.ng-1)]

src = opener(args.src, 'rb')
tgt = opener(args.tgt, 'rb')
al = opener(args.al, 'rb')
stag = opener(args.stag, 'rb') if args.stag else None
ttag = opener(args.ttag, 'rb') if args.ttag else None

lno, exno = 0, 0
for sline in src:
   stoks = sline.split()
   ttoks = tgt.next().split()
   atoks = al.next().split()
   stags = stag.next().split() if args.stag else None
   ttags = ttag.next().split() if args.ttag else None
   if args.stag and len(stoks) != len(stags):
      error("wrong number of source tags at line " + str(lno+1))
   if args.ttag and len(ttoks) != len(ttags):
      error("wrong number of target tags at line " + str(lno+1))

   sind = [get_index(svoc, i, stoks, stags, unk, beg) for i in range(len(stoks))]
   tind = [get_index(tvoc, i, ttoks, ttags, unk, beg) for i in range(len(ttoks))]
   oind = [get_index(ovoc, i, ttoks, ttags, unk, beg) for i in range(len(ttoks))]

   # read alignment and convert to target position -> src position mapping
   aind = [[] for e in tind]  # tpos -> spos list
   broken = False
   for tok in atoks:
      st = [int(x) for x in tok.split("-")]  # [spos,tpos]
      assert(len(st) == 2)
      if args.rev: st[0],st[1] = st[1],st[0]
      if st[0] >= len(sind) and not broken:
         log("Warning: skipping sent {} due to source length mismatch: {}".format(lno, sline))
         broken = True
      if st[1] >= len(tind) and not broken:
         log("Warning: skipping sent {} due to target length mismatch: {}".format(lno," ".join(ttoks)))
         broken = True
      if not broken:
         aind[st[1]].append(st[0])
   if not broken:
      if args.v > 1: # verbose check
         for i,e in enumerate(ttoks):
            print >> sys.stderr, e + '/' + ','.join([stoks[j] for j in aind[i]]),
         print >> sys.stderr, ""
      for i in reversed(range(len(aind))): # reverse iter due to rightward dep
         aind[i].sort()
         numals = len(aind[i])
         if numals == 0:
            aind[i] = aind[i+1] if i < len(aind)-1 else max(0,len(sind)-1)
         else:
            aind[i] = aind[i][(numals-1)/2]  # centre alignment, rounding down

      # create examples: src-window / ngram / w
      spad = lpad + ([str(e) for e in sind] if len(sind) else [str(bos)]) + rpad
      tpad = ngpad + [str(e) for e in tind]
      for i in range(len(tind)):
         print ' '.join(spad[aind[i]:aind[i]+args.ws]), '/',
         print ' '.join(tpad[i:i+args.ng-1]), '/', oind[i]
         exno += 1
      if args.eos: # assume alignment to source eos
         print ' '.join(spad[-args.ws+1:]), eos, '/', 
         print ' '.join(tpad[-args.ng+1+len(tpad):]), '/', eos
         exno += 1

   lno += 1
   if args.v and lno % 50000 == 0: print >> sys.stderr, '.',

if args.v:
   log("\n" + str(lno) + " lines read; " + str(exno) + " examples written")

if not args.r:
   log("writing vocabulary files")
   for n,v in ((args.voc+'.src',svoc), (args.voc+'.tgt',tvoc), (args.voc+'.out',ovoc)):
      with open(n, 'wb') as f:
         for i,t in enumerate(special_names):
            print >> f, i, t
         for w,i in sorted(v.items(), key = lambda x: x[1]):
            print >> f, i, w
