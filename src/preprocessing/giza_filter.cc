// $Id$
/**
 * @author Samuel Larkin
 * @file giza_filtre.cc
 * @brief Filter out sentence pairs that are problematic for giza.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
giza_filter [options] src_in tgt_in src_out tgt_out\n\
\n\
  Filter out sentence pairs for giza's input.\n\
  Drops sentence pairs where either:\n\
    - one sentence is empty;\n\
    - one has more than 100 tokens;\n\
    - the token ratio is greater than 9.\n\
\n\
Options:\n\
\n\
  -m M  drop sentence pairs where one of the sentence is longer than M [100]\n\
  -r R  drop sentence pairs where the length ratio is greater or equal to R [9]\n\
\n\
  -v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static Uint max_length(100);
static float max_ratio(9.0f);
static iMagicStream insrc;
static iMagicStream intgt;
static oMagicStream outsrc;
static oMagicStream outtgt;
static void getArgs(int argc, char* argv[]);

/// DummyConverter for calling split to simply count tokens.
struct DummyConverter {
   void operator()(const char* src, char& dest) { dest = 1; }
};

// main

int main(int argc, char* argv[])
{
   printCopyright(2012, "giza_filter");
   getArgs(argc, argv);

   Uint num_filter_out_sentence_pairs = 0;
   string src, tgt;
   vector<char> tokens;  ///< The current token indicators in that current line.
   while (getline(insrc, src) and getline(intgt, tgt)) {
      tokens.clear();
      const Uint num_src_token = split(src.c_str(), tokens, DummyConverter());
      tokens.clear();
      const Uint num_tgt_token = split(tgt.c_str(), tokens, DummyConverter());
      if (num_src_token == 0 or
          num_tgt_token == 0 or
          num_src_token > 100 or
          num_tgt_token > 100 or
          (float) num_src_token / (float) num_tgt_token >= 9.0f or
          (float) num_tgt_token / (float) num_src_token >= 9.0f) {
         ++num_filter_out_sentence_pairs;
         if (verbose)
            error(ETWarn, "Dropping src(%d): %s\n\ttgt(%d): %s",
               num_src_token, src.c_str(),
               num_tgt_token, tgt.c_str());
      }
      else {
         outsrc << src << '\n';
         outtgt << tgt << '\n';
      }
   }
   if (getline(insrc, src)) error(ETFatal, "The source file is longer than the target file.");
   if (getline(intgt, tgt)) error(ETFatal, "The target file is longer than the source file.");

   cerr << num_filter_out_sentence_pairs << " sentence pairs were dropped." << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "n:", "m:", "r:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("m", max_length);
   arg_reader.testAndSet("r", max_ratio);

   arg_reader.testAndSet(0, "insrc", insrc);
   arg_reader.testAndSet(1, "intgt", intgt);
   arg_reader.testAndSet(2, "outsrc", outsrc);
   arg_reader.testAndSet(3, "outtgt", outtgt);
}
