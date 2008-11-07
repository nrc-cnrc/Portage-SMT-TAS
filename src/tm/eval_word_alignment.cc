/**
 * @author George Foster
 * @file eval_word_alignment  Evaluate word alignment quality
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */
#include "file_utils.h"
#include "quick_set.h"
#include "word_align.h"
#include "word_align_io.h"
#include "arg_reader.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
eval_word_alignment [-vc][-fin fmt][-a a] text1 text2 ref al\n\
\n\
Evaluate word alignment <al> with respect to alignment <ref>. Both alignments\n\
pertain to line-aligned text files <text1> and <text2>.\n\
\n\
NB: If using Hwa format, set al_in and/or al_out to '-'. Hwa-style input is \n\
read from files aligned-in.<sent-num>.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-c    Compute the closure of the alignments, and add missing links before \n\
      evaluating [don't]\n\
-a    Use <a> in F-measure calculation: 1 / (a/prec + (1-a)/rec) [0.6]\n\
-fin  Format for <ref> and <al>, one of: "WORD_ALIGNMENT_READER_FORMATS" [green]\n\
";

// globals

static bool verbose = false;
static bool do_closure = false;
static string fin = "green";
static string text1, text2, ref, al;
static double alpha = 0.6;

static void getArgs(int argc, char* argv[]);

// main

int main(int argc, char* argv[])
{
   getArgs(argc, argv);

   iSafeMagicStream text1_file(text1);
   iSafeMagicStream text2_file(text2);
   iSafeMagicStream ref_file(ref);
   iSafeMagicStream al_file(al);

   WordAlignmentReader* wal_reader = WordAlignmentReader::create(fin);

   string line1, line2;
   vector<string> toks1, toks2;

   vector< vector<Uint> > sets_ref, sets_al;
   vector< vector<Uint> > csets_ref, csets_al;

   Uint line_num = 0;
   Uint ref_links = 0;
   Uint al_links = 0;
   Uint corr_links = 0;
   
   while (getline(text1_file, line1)) {

      ++line_num;
      if (!getline(text2_file, line2))
         error(ETFatal, "file %s too short", text2.c_str());
      splitZ(line1, toks1);
      splitZ(line2, toks2);

      (*wal_reader)(ref_file, toks1, toks2, sets_ref);
      (*wal_reader)(al_file, toks1, toks2, sets_al);

      if (do_closure) {
         WordAligner::close(sets_ref, csets_ref);
         WordAligner::close(sets_al, csets_al);
      }

      for (Uint i = 0; i < sets_ref.size(); ++i) {
         ref_links += sets_ref[i].size();
         if (i < sets_al.size())
            for (Uint j = 0; j < sets_ref[i].size(); ++j)
               if (find(sets_al[i].begin(), sets_al[i].end(), sets_ref[i][j])
                   != sets_al[i].end()) 
                  ++corr_links;
      }
      for (Uint i = 0; i < sets_al.size(); ++i)
         al_links += sets_al[i].size();
   }

   double prec = corr_links / (double) al_links;
   double rec = corr_links / (double) ref_links;
   double f = 1.0 / (alpha/prec + (1.0-alpha)/rec);

   cout << "prec = " << corr_links << "/" << al_links << " = " << prec << endl;
   cout << "rec =  " << corr_links << "/" << ref_links << " = " << rec << endl;
   cout << "f1(" << alpha << ") = " << f << endl;

}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "c", "fin:", "a:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("c", do_closure);
   arg_reader.testAndSet("fin", fin);
   arg_reader.testAndSet("a", alpha);

   arg_reader.testAndSet(0, "text1", text1);
   arg_reader.testAndSet(1, "text2", text2);
   arg_reader.testAndSet(2, "ref", ref);
   arg_reader.testAndSet(3, "al", al);
}
