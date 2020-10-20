#!/usr/bin/env python2
# -*- coding: utf-8 -*- 

# @file apply_ar_tweet_oov_rules.py
# @brief Apply new Arabic tweet preprocessing rules to raw Arabic OOVs.
#
# @author Darlene A. Stewart
#
# Traitement multilingue de textes / Multilingual Text Processing
# Tech. de l'information et des communications / Information and Communications Tech.
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada

"""
This script applies rules to transform OOVs found in Arabic tweet data into
proper Arabic text.

apply_ar_tweet_oov_rules.py implements a refined set of rules. This updated set
of rules was defined based on an analysis of applying the previous implementation
(rules_tagger_fixed.py) and Norah Alkharashi's analysis of the application
of Mo Al-Digeil's original implementation (rules_tagger_old.py) to OOVs
obtained from a set of Arabic tweets.

This script can be used to aid in assessing the effectiveness of the rules in
addressing problems found in Arabic tweets and reducing the number of OOVs.
"""

from __future__ import print_function, unicode_literals, division, absolute_import

import sys
from argparse import ArgumentParser, FileType
import codecs
import re
import os
import subprocess
import json

# Make sure the location of portage_utils is in your PYTHONPATH.
from portage_utils import *

def get_args():
   """Command line argument processing."""

   usage="apply_ar_tweet_oov_rules.py [options] [infile [outfile]]"
   help="""
   Apply Arabic tweet preprocessing rules to Arabic OOVs in either the original
   Arabic text or buckwalter transliterated text.
   
   The script can also produce a rules output file in which each OOV is
   identified by word count, each rule applied is identified by the rule ID,
   and both the original and transformed text are output.
   """

   # Use the argparse module, not the deprecated optparse module.
   parser = ArgumentParser(usage=usage, description=help, add_help=False)

   # Use our standard help, verbose and debug support.
   parser.add_argument("-h", "-help", "--help", action=HelpAction)
   parser.add_argument("-v", "--verbose", action=VerboseAction)
   parser.add_argument("-d", "--debug", action=DebugAction)

   translit = parser.add_mutually_exclusive_group()
   translit.add_argument('-bw', '--buckwalter', dest="buckwalter",
                         action='store_true', default=True,
                         help="Select buckwalter transliteration. [True]")
   translit.add_argument('-ar', '--arabic', dest="buckwalter",
                         action='store_false', default=True,
                         help="Select raw Arabic text. [False]")

   parser.add_argument("-oov", "-t", "--oovtags", dest="oov_tags",
                       action='store_true', default=False,
                       help='''OOVs are marked up in the input by XML tags,
                            i.e.<oov>oov-phrase</oov>. OOV tags are removed in
                            the output. [%(default)s]''')

   parser.add_argument("-r", "--rules", dest="rulesfile", nargs='?',
                       type=FileType('w'), const=sys.stderr, default=None,
                       help='''File to output applied rules info to.
                            [None if no option, sys.stderr if no filename]''')

   parser.add_argument("-m", "--map", dest="mapfile", type=str, default=None,
                       help='''MADA Map file to use with -bw.
                            [search plugins + PERL5LIB for tokenize/Arabic/Data.pm]''')

   # The following use the portage_utils version of open to open files.
   parser.add_argument("infile", nargs='?', type=open, default=sys.stdin,
                       help="input file [sys.stdin]")
   parser.add_argument("outfile", nargs='?', type=lambda f: open(f,'w'),
                       default=sys.stdout,
                       help="output file [sys.stdout]")

   cmd_args = parser.parse_args()

   return cmd_args

word2morpho = {}     # Name chosen to parallel Perl code usage.

def loadMappingData(mapfile):
   """Loaded the MADA Map data from the specified file or the Perl
   tokenize/Arabic/Data.pm module.

   mapfile: MADA Map file pathname; None triggers searching the plugins and
      PERL5LIB for tokenize/Arabic/Data.pm. If the mapfile ends in '.json' then
      it is treated as a pure JSON file, not a Perl module with a __Data__ tag.
   """
   global word2morpho
   if mapfile is None:
      # First look for the map relative to the plugins location, then in PERL5LIB
      which_cmd = 'which tokenize_plugin_ar 2> /dev/null; exit 0'
      perl5lib = [os.path.dirname(subprocess.check_output(which_cmd, shell=True))]
      perl5lib += os.environ['PERL5LIB'].split(':')
      for libdir in perl5lib:
         if not libdir: continue
         mapfile = os.path.join(libdir, "tokenize", "Arabic", "Data.pm")
         if os.path.exists(mapfile):
            break
      else:
         error("tokenize/Arabic/Data.pm module not found.")
   else:
      if mapfile == '/dev/null':
         info("Not using MADA Map (/dev/null).")
         return
   info("Using MADA Map:", mapfile)
   if mapfile.endswith(".json"):
      data_pipe = open(mapfile)
   else:
      # The JSON data follows the __DATA__ tag in the Data.pm file.
      cmd = "sed -e '1,/__DATA__/d' " + mapfile
      data_pipe = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE).stdout
   word2morpho = json.load(data_pipe)
   data_pipe.close()

def main():
   cmd_args = get_args()

   # Allow the file streams to handle non-ascii characters.
   infile = codecs.getreader("utf-8")(cmd_args.infile)
   outfile = codecs.getwriter("utf-8")(cmd_args.outfile)
   if cmd_args.rulesfile is None:
      rulesfile = None
   else:
      rulesfile = codecs.getwriter("utf-8")(cmd_args.rulesfile)
   sys.stderr = codecs.getwriter("utf-8")(sys.stderr)

   if cmd_args.buckwalter:
      loadMappingData(cmd_args.mapfile)

   # New rule 1.0 is old rule 2-1.
   # New rule 2.0 is old rule 2-2.
   # New rule 3.0 is old (fixed) rule 2-3.
   # New rule 3.1 handles words of old rules 2-6/2-8 with old (fixed) rule 2-3 placement.
   # New rule 3.2 handles words of old rules 2-5/2-7 with old (fixed) rule 2-3 placement.
   # New rule 3.3 adds handling of vocative particle using old (fixed) rule 2-3 placement.
   # New rule 3.4 adds handling of pronoun (this) using old (fixed) rule 2-3 placement.
   # New rule 4.0 is old (fixed) rule 2-4.
   # New rule 5.0 extends old (fixed) rules 3-1/3-2 to all Arabic letters repeated >2 times.
   # New rule 5.1 extends old (fixed) rules 3-1/3-2 to a particular repeated letter-pair (LAM + ALIF).
   # New rule 5.2 extends old (fixed) rules 3-1/3-2 to long vowels YEH as well as WAW
   # repeated twice or more. (ALIF is moved to rule 5.3)
   # New rule 5.3 is rule 5.2 for ALIF.
   # Notes: not-start-of-word: (?<!^)(?<! )  not-end-of-word: (?! )(?!$)

   # Using a literal initializer for pat_ar doesn't work very well because we
   # end up with the lines displaying left to right in some tools.
   # Assignment statements with only a single, short Arabic word display
   # left-to-right, as expected.
   # pat_ar = {
   #    "1.0": ( "ى", "ء", "ئ", "ة", ),
   #    "2.0": ( "ال", ),
   #    "3.0": ( "لا", "و", ),
   #    "3.1": ( "إذا", "لذا", "ما", ),
   #    "3.2": ( "اً", "أن",),
   #    "3.3": ( "يا", ),
   #    "3.4": ( "هذا", ),
   #    "4.0": ( "د", "ذ", "ر", "ز", "و", ),
   #    "5.1": ( "لا", ),
   #    "5.2": ( "و", "ي", ),
   #    "5.3": ( "ا", ),
   # }

   pat_ar = {}
   ar = pat_ar["1.0"] = [""] * 4
   ar[0] = "ى"
   ar[1] = "ء"
   ar[2] = "ئ"
   ar[3] = "ة"
   ar = pat_ar["2.0"] = [""] * 1
   ar[0] = "ال"
   ar = pat_ar["3.0"] = [""] * 2
   ar[0] = "و"
   ar[1] = "لا"
   ar = pat_ar["3.1"] = [""] * 3
   ar[0] = "إذا"
   ar[1] = "لذا"
   ar[2] = "ما"
   ar = pat_ar["3.2"] = [""] * 2
   ar[0] = "اً"
   ar[1] = "أنا"
   ar = pat_ar["3.3"] = [""] * 1
   ar[0] = "يا"
   ar = pat_ar["3.4"] = [""] * 1
   ar[0] = "هذا"
   ar = pat_ar["4.0"] = [""] * 5
   ar[0] = "د"
   ar[1] = "ذ"
   ar[2] = "ر"
   ar[3] = "ز"
   ar[4] = "و"
   pat_ar["5.0"] = ["[\u0620-\u064A]"]
   ar = pat_ar["5.1"] = [""] * 1
   ar[0] = "لا"
   ar = pat_ar["5.2"] = [""] * 2
   ar[0] = "و"
   ar[1] = "ي"
   ar = pat_ar["5.3"] = [""] * 1
   ar[0] = "ا"

   # Buckwalter pattern notes:
   # - We must escape characters with special meaning in Python re patterns,
   #   hence the use of raw strings and '\' before some characters.
   # - We expect the buckwalter characters '<' and '>' to be backslash
   #   escaped; we handle the backslashes outside the rules.
   pat_bw = {
      "1.0": ( "Y", "'", r"\}", "p", ),
      "2.0": ( "Al", ),
      "3.0": ( "w", "lA", ),
      "3.1": ( r"<\*A", r"l\*A", "mA", ),
      "3.2": ( "AF", ">nA", ),
      "3.3": ( "yA", ),
      "3.4": ( r"h\*A", ),
      "4.0": ( "d", r"\*", "r", "z", "w", ),
      "5.0": ( "[a-zA-Z$&'*<>_`{|}~]", ),
      "5.1": ( "lA", ),
      "5.2": ( "w", "y", ),
      "5.3": ( "A", ),
   }

   pat_txt = pat_bw if cmd_args.buckwalter else pat_ar

   pat = {}

   # Rules 1.0 and 2.0 need to be applied only once, where the matched text
   # occurs in the middle of a token.
   for rule_id in ("1.0", "2.0"):
      pat[rule_id] = "(?<!^)(?<! )(" + '|'.join(pat_txt[rule_id]) + ")(?! )(?!$)"

   # Rules 3.* are applied repeatedly to tokens where the matched text is preceded
   # by at most 1 character in the word and followed by at least two characters.
   for rule_id in ("3.0", "3.1", "3.2", "3.3", "3.4"):
      pat[rule_id] = "(?<![^ ]{2})(" + '|'.join(pat_txt[rule_id]) + ")(?=[^ ]{2})"

   # For application of rule 4.0, the updated OOV text is retokenized, and the
   # substitution is applied only to tokens of length > 7 characters.
   pat["4.0"] = "(?<!^)(" + '|'.join(pat_txt["4.0"]) + ")(?!$)"

   # Rules 5.* merely remove repeated characters, thus have no tokenization effect.
   pat["5.0"] = "(" + '|'.join(pat_txt["5.0"]) + r")\1\1+"
   for rule_id in ("5.1", "5.2", "5.3"):
      pat[rule_id] = "(" + '|'.join(pat_txt[rule_id]) + r")\1+"

   rules = {
      "1.0": (re.compile(pat["1.0"]), r"\1 "),
      "2.0": (re.compile(pat["2.0"]), r" \1"),
      "3.0": (re.compile(pat["3.0"]), r"\1 "),
      "3.1": (re.compile(pat["3.1"]), r"\1 "),
      "3.2": (re.compile(pat["3.2"]), r"\1 "),
      "3.3": (re.compile(pat["3.3"]), r"\1 "),
      "3.4": (re.compile(pat["3.4"]), r"\1 "),
      "4.0": (re.compile(pat["4.0"]), r"\1 "),
      "5.0": (re.compile(pat["5.0"]), r"\1"),
      "5.1": (re.compile(pat["5.1"]), r"\1"),
      "5.2": (re.compile(pat["5.2"]), r"\1"),
      "5.3": (re.compile(pat["5.3"]), r"\1"),
   }

   # The rules are applied in order according to the cat_N_rules lists.
   # Category 1 rules are applied first, each applied once.
   # Category 2 rules are then applied repeatedly until no more changes result;
   # at most 1 letter precede and at least 2 letters follow the matched text.
   # Category 1b exists to postpone the application of rule 5 for double Alif
   # until after category 2 rules are applied, which may split the double Alif.
   # Category 3 rules are applied last, each applied once to tokens of length > 7.
   cat_1_rules = ("5.0", "5.1", "5.2", "1.0", "2.0",)
   cat_2_rules = ("3.0", "3.1", "3.2", "3.3", "3.4",)
   cat_1b_rules = ("5.3",)
   cat_3_rules = ("4.0",)

   rule_cnts_oov = {}
   rule_cnts_appl = {}
   rule_cnts_all = {}
   for rule_id in rules:
      rule_cnts_oov[rule_id] = 0
      rule_cnts_appl[rule_id] = 0
      rule_cnts_all[rule_id] = 0

   def print_rule(rule_id, text):
      if rulesfile is not None:
         print(oov_count, 'm', rule_id, ' ', text, sep='', file=rulesfile)

   def apply_rule(rule_id, text, update_oov_cnt, do_print):
      """
      Apply the specified rule to some text, printing the result if requested.

      rule_id: ID of the rule to apply, e.g. "3.1"
      text: text string to apply the rule to; can contain one or more words.
      update_oov_cnt: update the OOV count for the rule if True.
      do_print: outputs rule tag and updated text if True.
      returns a tuple consisting of the updated text and the number of changes.
      """
      pat, repl = rules[rule_id]
      updated_text, n = re.subn(pat, repl, text)
      if n > 0:
         if do_print:
            print_rule(rule_id, updated_text)
         if update_oov_cnt: rule_cnts_oov[rule_id] += 1
         rule_cnts_appl[rule_id] += 1
         rule_cnts_all[rule_id] += n
      return updated_text, n

   def apply_rules(oov):
      """Apply rules to an OOV returning the updated oov text."""
      print_rule('0.0', oov)
      updated_text = oov

      # For buckwalter text, temporarily remove the XML escapes, so letter
      # counts for rules will be correct.
      # i.e. '&gt;' -> '>', '&lt;' -> '<', &amp;' -> '&'.
      if cmd_args.buckwalter and cmd_args.oov_tags:
         updated_text, n_esc_gt = re.subn('&gt;', '>', updated_text)
         updated_text, n_esc_lt = re.subn('&lt;', '<', updated_text)
         updated_text, n_esc_amp = re.subn('&amp;', '&', updated_text)

      # Category 1 rules need to be applied only once.
      for rule_id in cat_1_rules:
         updated_text, n = apply_rule(rule_id, updated_text, True, True)

      # Category 2 rules need to be re-applied until no more changes occur.
      num_changes = -1
      first_time = True
      while first_time or num_changes != 0:
         num_changes = 0
         for rule_id in cat_2_rules:
            updated_text, n = apply_rule(rule_id, updated_text, first_time, True)
            num_changes += n
         first_time = False

      # Category 1b rules need to be applied only once.
      for rule_id in cat_1b_rules:
         updated_text, n = apply_rule(rule_id, updated_text, True, True)

      # Category 3 rules need to be applied once only to words with length > 7.
      upd_wrds = split(updated_text)
      for rule_id in cat_3_rules:
         pat, repl = rules[rule_id]
         num_changes = 0
         for i, wrd in enumerate(upd_wrds):
            if len(wrd) > 7:
               upd_wrds[i], n = apply_rule(rule_id, wrd, num_changes==0, False)
               num_changes += n
         if num_changes > 0:
            updated_text = ' '.join(upd_wrds)
            print_rule(rule_id, updated_text)

      # Re-apply the MADA Map if operating on buckwalter transliterated text.
      if cmd_args.buckwalter and updated_text != oov:
         updated_text = ' '.join(word2morpho.get(wrd, wrd) for wrd in split(updated_text))

      # For buckwalter text, restore previously removed XML escapes
      # i.e. '&gt;' -> '>', '&lt;' -> '<', &amp;' -> '&'.
      if cmd_args.buckwalter and cmd_args.oov_tags:
         updated_text, n = re.subn('&', '&amp;', updated_text)
         updated_text, n = re.subn('>', '&gt;', updated_text)
         updated_text, n = re.subn('<', '&lt;', updated_text)

      print_rule('X.X', updated_text)
      return updated_text

   line_number = 0;
   oov_count=0

   for line in infile:
      line_number += 1
      out_line = []
      if not cmd_args.oov_tags:
         oov_count += 1
         out_line.append(apply_rules(line.strip()))
      else:
         oov_start = oov_end = False
         for token in split(line):
            if token.startswith("<oov>"):
               token = token[5:]
               oov_start = True
            if token.endswith("</oov>"):
               token = token[:-6]
               oov_end = True
            if not oov_start:
               out_line.append(token)
            else:
               if len(token) > 0:
                  oov_count += 1
                  out_line.append(apply_rules(token))
            if oov_end:
               oov_start = oov_end = False
         if oov_start:
            error("Missing </oov> for oov", oov_count, "in line", line_number)
      print(*out_line, file=outfile)

   # Print the stats
   verbose("Rules applied (# OOV words)")
   for rule_id in sorted(rule_cnts_oov.keys()):
      verbose(rule_id, ": ", rule_cnts_oov[rule_id], sep='')
   verbose("Rules applied (# rule applications)")
   for rule_id in sorted(rule_cnts_appl.keys()):
      verbose(rule_id, ": ", rule_cnts_appl[rule_id], sep='')
   verbose("Rules applied (total # changes)")
   for rule_id in sorted(rule_cnts_all.keys()):
      verbose(rule_id, ": ", rule_cnts_all[rule_id], sep='')


if __name__ == '__main__':
   main()
