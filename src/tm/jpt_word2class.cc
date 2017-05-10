/**
 * @author Darlene Stewart
 * @file jpt_word2class.cc
 * @brief  Program to map words to word classes in a JPT, replacing each word
 * in a phrase by its corresponding word class.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "file_utils.h"
#include "arg_reader.h"
#include "phrase_table.h"
#include "word_classes.h"
#include "errors.h"
#include "printCopyright.h"
//#include <algorithm>

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
jpt_word2class [options] JPT_IN SRC_CLASSES TGT_CLASSES [JPT_OUT]\n\
\n\
  Map words in JPT_IN to word classes, replacing each source phrase word by its\n\
  corresponding word class from SRC_CLASSES and each target phrase word by its\n\
  corresponding word class from TGT_CLASSES writing the result to JPT_OUT.\n\
\n\
Options:\n\
\n\
  -h    Print this help message\n\
  -v    Print progress reports to cerr\n\
  -d    Print debug information to cerr\n\
";

static bool verbose = false;
static bool debug = false;
static string jpt_in_file;
static string src_classes_file;
static string tgt_classes_file;
static string jpt_out_file("-");

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "d"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", debug);

   arg_reader.testAndSet(0, "JPT_IN", jpt_in_file);
   arg_reader.testAndSet(1, "SRC_CLASSES", src_classes_file);
   arg_reader.testAndSet(2, "TGT_CLASSES", tgt_classes_file);
   arg_reader.testAndSet(3, "JPT_OUT", jpt_out_file);
}


int main(int argc, char* argv[])
{
   printCopyright(2013, "jpt_word2class");
   getArgs(argc, argv);
   iSafeMagicStream jpt_in(jpt_in_file);
   WordClasses src_classes;
   WordClasses tgt_classes;
   oSafeMagicStream jpt_out(jpt_out_file);

   src_classes.read(src_classes_file);
   tgt_classes.read(tgt_classes_file);

   const string psep = " " + PhraseTableBase::psep + " ";
   Uint lineno = 0;
   Uint src_map_errors = 0;
   Uint src_tok_count = 0;
   Uint tgt_map_errors = 0;
   Uint tgt_tok_count = 0;
   string line;
   PhraseTableBase::ToksIter b1, e1, b2, e2, v, a, f;
   vector<char*> toks;
   vector<Uint> sc, tc;
   while (getline(jpt_in, line)) {
      ++lineno;

      if (debug)
         cerr << "in(" << lineno << "): " << line << nf_endl;

      char buffer[line.size()+1];
      strcpy(buffer, line.c_str());
      PhraseTableBase::extractTokens(line, buffer, toks, b1, e1, b2, e2, v, a, f);

      sc.clear();
      for (PhraseTableBase::ToksIter it=b1; it < e1; ++it) {
         const Uint cl(src_classes.classOf(*it));
         if (cl == WordClasses::NoClass)
            ++src_map_errors;
         sc.push_back(cl);
      }
      src_tok_count += e1 - b1;

      tc.clear();
      for (PhraseTableBase::ToksIter it=b2; it < e2; ++it) {
         const Uint cl(tgt_classes.classOf(*it));
         if (cl == WordClasses::NoClass)
            ++tgt_map_errors;
         tc.push_back(cl);
      }
      tgt_tok_count += e2 - b2;

      jpt_out << join(sc) << psep << join(tc) << psep << join(v,a) << nf_endl;
   }

   jpt_out.flush();

   if (src_map_errors > 0)
      cerr << src_map_errors << " source word mapping errors." << endl;
   cerr << src_tok_count-src_map_errors << " of " << src_tok_count << " source words mapped to word classes." << endl;
   if (tgt_map_errors > 0)
      cerr << tgt_map_errors << " target word mapping errors." << endl;
   cerr << src_tok_count-tgt_map_errors << " of " << src_tok_count << " target words mapped to word classes." << endl;
   if (src_map_errors > 0 || tgt_map_errors > 0)
      error(ETFatal, "Output JPT contains %d word class mapping errors.",
            src_map_errors+tgt_map_errors);
}

