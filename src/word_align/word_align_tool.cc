/**
 * @author George Foster
 * @file word_align_tool.cc
 * @brief Reformat and perform other operations on word alignments.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <set>
#include <file_utils.h>
#include <quick_set.h>
#include "word_align.h"
#include "word_align_io.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
word_align_tool  [-vct][-fin fmt][-fout fmt][-crp file] text1 text2 al_in al_out\n\
\n\
Read two line-aligned text files and associated word alignments <al_in> in a\n\
given format, perform specified operations, and write new alignment file\n\
<al_out>, possibly in a different format.\n\
\n\
NB1: If using Hwa format for input and/or output, set al_in and/or al_out to\n\
'-'. Hwa-style input is read from files aligned-in.<sent-num>, and written to\n\
files aligned.<sent-num>.\n\
\n\
NB2: Not all formats preserve information about the distinction between\n\
unaligned and untranslated words, so round-tripping can change alignments.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-c    Compute the closure of the alignments, and add missing links [don't]\n\
-t    Transpose the output alignment, swapping the roles of text1 and text2\n\
      [don't]\n\
-fin  Format for input alignment file, one of: "WORD_ALIGNMENT_READER_FORMATS" [hwa]\n\
-fout Format for output alignment file, one of:\n\
      "WORD_ALIGNMENT_WRITER_FORMATS",\n\
      or patterns [green]\n\
      Note: type \"align-words -H\" for documentation on each format.\n\
-crp  Write Uli-style combined text file to <file>.\n\
";

// globals

static bool verbose = false;
static bool do_closure = false;
static bool do_transpose = false;
static string fin = "hwa";
static string fout = "green";
static string crp;
static string text1, text2, al_in, al_out;

static void getArgs(int argc, char* argv[]);
static void transpose(vector<string>& toks1, vector<string>& toks2, vector< vector<Uint> >& sets1);
static void writePatterns(ostream& al_out_file,
                          const vector<string>& toks1, const vector<string>& toks2,
                          vector< vector<Uint> >& sets,
                          vector< vector<Uint> >& csets);


// main

int main(int argc, char* argv[])
{
   printCopyright(2007, "word_align_tool");
   getArgs(argc, argv);

   iSafeMagicStream text1_file(text1);
   iSafeMagicStream text2_file(text2);
   iSafeMagicStream al_in_file(al_in);
   oSafeMagicStream al_out_file(al_out);

   ostream* crp_file = NULL;
   if (crp != "")
      crp_file = new oSafeMagicStream(crp);

   WordAlignmentReader* wal_reader = WordAlignmentReader::create(fin);
   WordAlignmentWriter* wal_writer = fout == "patterns" ?
      NULL : WordAlignmentWriter::create(fout);
   assert(wal_reader);

   if (fout == "patterns") do_closure = true;

   string line1, line2;
   vector<string> toks1, toks2;

   vector< vector<Uint> > sets1;
   vector< vector<Uint> > csets;

   Uint line_num = 0;

   while (getline(text1_file, line1)) {
      ++line_num;
      if (!getline(text2_file, line2))
         error(ETFatal, "file %s too short", text2.c_str());
      splitZ(line1, toks1);
      splitZ(line2, toks2);

      if (!(*wal_reader)(al_in_file, toks1, toks2, sets1))
         error(ETFatal, "alignment file %s too short", al_in.c_str());

      if (do_transpose) transpose(toks1, toks2, sets1);
      if (do_closure) WordAligner::close(sets1, csets);

      if (wal_writer)
         (*wal_writer)(al_out_file, toks1, toks2, sets1);
      else
         writePatterns(al_out_file, toks1, toks2, sets1, csets);

      if (crp_file) {
         const string& l1 = do_transpose ? line2 : line1;
         const string& l2 = do_transpose ? line1 : line2;
         (*crp_file) << line_num << endl << l1 << endl << l2 << endl;
      }
   }
   delete wal_reader;
   delete wal_writer;
   delete crp_file;
}

// this is expensive

void transpose(vector<string>& toks1, vector<string>& toks2, vector< vector<Uint> >& sets1)
{
   vector< vector<Uint> > sets2(toks2.size());
   for (Uint i = 0; i < sets1.size(); ++i) {
      for (Uint j = 0; j < sets1[i].size(); ++j) {
         if (sets1[i][j] == sets2.size()) sets2.push_back(vector<Uint>());
         sets2[sets1[i][j]].push_back(i);
      }
   }

  // the expensive part
   sets1 = sets2;
   swap(toks1, toks2);
}

// Produce an alignment pattern; pos_set is assumed to be sorted ascending

string& alignPattern(const vector<Uint>& pos_set, Uint ntoks, string& str,
                     const vector<string>* toks = NULL)
{
   assert(pos_set.size());
   str.clear();

   if (pos_set.size() == 1 && pos_set[0] == ntoks)
      str = toks ? "[[E]]" : "E"; // special case
   else {
      str = toks ? (*toks)[pos_set[0]] : "A";
      for (Uint i = 1; i < pos_set.size(); ++i) {
         if (pos_set[i-1]+1 != pos_set[i])
            str += toks ? " [[X]]" : "XA";
         if (toks)
            str += " " + (*toks)[pos_set[i]];
      }
   }
   return str;
}

// NB: contents of sets and csets are sorted ascending in place

void writePatterns(ostream& al_out_file,
                   const vector<string>& toks1, const vector<string>& toks2,
                   vector< vector<Uint> >& sets,
                   vector< vector<Uint> >& csets)
{
   static Uint line_num = 0;

   string s1, s2;

   ++line_num;

   for (Uint i = 0; i < csets.size(); ++i) {

      sort(csets[i].begin(), csets[i].end());
      assert(csets[i].size());
      Uint k = csets[i][0];
      if (sets[k].empty())
         continue;   // don't bother writing unaligned words
      sort(sets[k].begin(), sets[k].end());

      // write patterns
      al_out_file << alignPattern(csets[i], toks1.size(), s1) << ':'
                  << alignPattern(sets[k], toks2.size(), s2) << ' ';

      // write 1-based line index
      al_out_file << line_num << ' ';

      // write L1 positions
      for (Uint j = 0; j < csets[i].size(); ++j) {
         al_out_file << csets[i][j]
                     << (j+1 < csets[i].size() ? ',' : ':');
      }
      // write L2 positions
      for (Uint j = 0; j < sets[k].size(); ++j)
         al_out_file << sets[k][j]
                     << (j+1 < sets[k].size() ? ',' : ' ');

      // write phrases
      al_out_file << "||| "
                  << alignPattern(csets[i], toks1.size(), s1, &toks1) << " ||| "
                  << alignPattern(sets[k], toks2.size(), s2, &toks2) << endl;
   }
}


// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "c", "t", "fin:", "fout:", "crp:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("c", do_closure);
   arg_reader.testAndSet("t", do_transpose);
   arg_reader.testAndSet("fin", fin);
   arg_reader.testAndSet("fout", fout);
   arg_reader.testAndSet("fout", fout);
   arg_reader.testAndSet("crp", crp);

   arg_reader.testAndSet(0, "text1", text1);
   arg_reader.testAndSet(1, "text2", text2);
   arg_reader.testAndSet(2, "al_in", al_in);
   arg_reader.testAndSet(3, "al_out", al_out);
}
