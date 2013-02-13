/**
 * @author Samuel Larkin
 * @file giza_convert_ttable.cc
 * @brief Convert a giza ttable into a format PortageII can read.
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
giza_convert_ttable [options] src_vocab tgt_vocab giza_ttable [OUT]\n\
\n\
  Convert a Giza ttable into a format PortageII can read.\n\
  Giza's format uses integer id for word where as PortageII's format uses\n\
  string for word.\n\
\n\
Options:\n\
\n\
  -v    Write progress reports to cerr.\n\
";

// globals

static bool verbose = false;
static string src_vocab_file;
static string tgt_vocab_file;
static string giza_ttable_file;
static string outfile("-");
static void getArgs(int argc, char* argv[]);

void loadVocabulary(vector<string>& voc, const string& filename) {
   voc.reserve(1000);
   voc.push_back("NULL");
   voc.push_back("UNK");
   iSafeMagicStream file(filename);
   string line;
   vector<string> tokens;
   assert(voc.size() == 2);
   while (getline(file, line)) {
      splitZ(line, tokens);
      if (tokens.size() != 3)
         error(ETFatal, "Invalid format of vocabulary %s %d", filename.c_str(), tokens.size());
      voc.push_back(tokens[1]);
   }
   Uint id(0);
   if (!conv(tokens[0], id))
      error(ETFatal, "Error converting last token's seq_id (%s, %s)", filename.c_str(), tokens[0].c_str());
   if (id+1 != voc.size())
      error(ETFatal, "Error reading the vocabulary file (%s, %d : %d)", filename.c_str(), id, voc.size());
}

// main

int main(int argc, char* argv[])
{
   printCopyright(2012, "giza_ttable_file");
   getArgs(argc, argv);

   vector<string> svocab;
   vector<string> tvocab;

   if (verbose > 0) cerr << "Reading source vocabulary file: " << src_vocab_file << endl;
   loadVocabulary(svocab, src_vocab_file);
   if (verbose > 0) cerr << "Reading target vocabulary file: " << tgt_vocab_file << endl;
   loadVocabulary(tvocab, tgt_vocab_file);

   iSafeMagicStream ttable(giza_ttable_file);
   oSafeMagicStream output(outfile);

   string line;
   vector<string> tokens;
   Uint src_id, tgt_id;
   if (verbose > 0) cerr << "Start processing giza's ttable: " << giza_ttable_file << endl;
   while (getline(ttable, line)) {
      splitZ(line, tokens);
      if (!conv(tokens[0], src_id))
         error(ETFatal, "Error converting source token id");
      if (!conv(tokens[1], tgt_id))
         error(ETFatal, "Error converting target token id");
      if (src_id >= svocab.size())
         error(ETFatal, "Error, source id not in vocabulary %d", src_id);
      if (tgt_id >= tvocab.size())
         error(ETFatal, "Error, target id not in vocabulary %d", tgt_id);
      output << svocab[src_id] << " " << tvocab[tgt_id] << " " << tokens[2] << endl;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);

   arg_reader.testAndSet(0, "src_vocab_file", src_vocab_file);
   arg_reader.testAndSet(1, "tgt_vocab_file", tgt_vocab_file);
   arg_reader.testAndSet(2, "giza_ttable_file", giza_ttable_file);
   arg_reader.testAndSet(3, "outfile", outfile);
}
