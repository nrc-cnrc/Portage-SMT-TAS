/**
 * @author Darlene Stewart
 * @file filter_word_classes.cc
 * @brief  Filter a word classes file, keeping only the mappings for words that
 * are also present in the provided text file (vocab).
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#include "file_utils.h"
#include "arg_reader.h"
#include "errors.h"
#include "printCopyright.h"
#include "voc.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
filter_word_classes [options] CLASSES VOCAB [OUT]\n\
\n\
  Filter CLASSES, keeping only the mappings for words that are also present\n\
  in VOCAB, writing the result to OUT.\n\
\n\
Options:\n\
\n\
  -h    Print this help message\n\
  -v    Print progress reports to cerr\n\
  -d    Print debug information to cerr\n\
";

static bool verbose = false;
static bool debug = false;
static string classes_file;
static string vocab_file;
static string out_file("-");

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "d"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", debug);

   arg_reader.testAndSet(0, "CLASSES", classes_file);
   arg_reader.testAndSet(1, "VOCAB", vocab_file);
   arg_reader.testAndSet(2, "OUT", out_file);
}


int main(int argc, char* argv[])
{
   printCopyright(2013, "filter_word_classes");
   getArgs(argc, argv);
   iSafeMagicStream f_vocab(vocab_file);
   iSafeMagicStream f_classes(classes_file);
   oSafeMagicStream f_out(out_file);

   Voc vocab;

   // Read the vocabulary file.
   string line;
   vector<string> toks;
   Uint v_lineno = 0;
   while (getline(f_vocab, line)) {
      ++v_lineno;
      splitZ(line, toks);
      for (vector<string>::iterator it = toks.begin(); it < toks.end(); ++it) {
         vocab.add(it->c_str());
      }
   }
   if (verbose)
      cerr << "Read " << v_lineno << " lines from vocab file." << endl;

   // Read the classes file, writing matching lines to the output file.
   Uint in_line_no(0);
   Uint out_line_no(0);
   while (getline(f_classes, line)) {
      ++in_line_no;
      splitZ(line, toks, "\t");
      if ( toks.size() != 2 )
         error(ETFatal, "Error in classes file %s line %d: "
               "expected exactly one tab character, found %d",
               classes_file.c_str(), in_line_no, toks.size()-1);
      if (vocab.index(toks[0].c_str()) != vocab.size()) {
         f_out << line << nf_endl;
         ++out_line_no;
      }
   }
   f_out.flush();

   if (verbose) {
      cerr << "Vocab size: " << vocab.size() << endl;
      cerr << "Kept " << out_line_no << " of " << in_line_no << " lines from classes file." << endl;
   }
}

