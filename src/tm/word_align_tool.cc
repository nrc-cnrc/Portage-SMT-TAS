/**
 * @author George Foster
 * @file word_align_tool.cc
 * @brief Reformat and perform other operations on word alignments.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <set>
#include <file_utils.h>
#include <quick_set.h>
#include "word_align_io.h"
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
word_align_tool  [-v][-fin fmt][-fout fmt][-c][-t] text1 text2 al_in al_out\n\
\n\
Read two line-aligned text files and associated word alignments <al_in> in a\n\
given format, perform specified operations, and write new alignment file\n\
<al_out>, possibly in a different format.\n\
\n\
NB1: If using Hwa format for input and/or output, set al_in and/or al_out to '-'. Hwa-style\n\
input is read from files aligned-in.<sent-num>, and written to files aligned.<sent-num>.\n\
\n\
NB2: Not all formats preserve information about the distinction between\n\
unaligned and untranslated words, so round-tripping can change alignments.\n\
\n\
Options:\n\
\n\
-v    Write progress reports to cerr.\n\
-fin  Format for input alignment file, one of: "WORD_ALIGNMENT_READER_FORMATS" [hwa]\n\
-fout Format for output alignment file, one of: "WORD_ALIGNMENT_WRITER_FORMATS", or patterns [green]\n\
-c    Compute the closure of the alignments, and add missing links [don't]\n\
-t    Transpose the output alignment, swapping the roles of text1 and text2 [don't]\n\
";

// globals

static bool verbose = false;
static string fin = "hwa";
static string fout = "green";
static bool do_closure = false;
static bool do_transpose = false;
static string text1, text2, al_in, al_out;

static void getArgs(int argc, char* argv[]);
static void closure(vector< vector<Uint> >& sets1, vector< vector<Uint> >& csets);
static void transpose(vector<string>& toks1, vector<string>& toks2, vector< vector<Uint> >& sets1);
static bool intersectionNotNull(QuickSet& qs, vector<Uint>& vs);
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

   WordAlignmentReader* wal_reader = WordAlignmentReader::create(fin);
   WordAlignmentWriter* wal_writer = fout == "patterns" ? 
      NULL : WordAlignmentWriter::create(fout);

   if (fout == "patterns") do_closure = true;
   
   string line1, line2;
   vector<string> toks1, toks2;

   vector< vector<Uint> > sets1;
   vector< vector<Uint> > csets;
   
   while (getline(text1_file, line1)) {
      if (!getline(text2_file, line2))
         error(ETFatal, "file %s too short", text2.c_str());
      splitZ(line1, toks1);
      splitZ(line2, toks2);

      (*wal_reader)(al_in_file, toks1, toks2, sets1);

      if (do_transpose) transpose(toks1, toks2, sets1);
      if (do_closure) closure(sets1, csets);

      if (wal_writer)
         (*wal_writer)(al_out_file, toks1, toks2, sets1);
      else
         writePatterns(al_out_file, toks1, toks2, sets1, csets);
   }
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

// is intersection between two sets of integers non-empty?

bool intersectionNotNull(QuickSet& qs, vector<Uint>& vs)
{
   for (Uint i = 0; i < vs.size(); ++i)
      if (qs.find(vs[i]) != qs.size())
         return true;
   return false;
}

/**
 * Determine the closure of the given word alignment: whenever there is an
 * alignment path between a source word and a target word, add a direct link
 * connecting the two. The algorithm exploits the standard representation that
 * maintains a set of connected L2 words for each L1 word. If the sets for two
 * different L1 words have at least one element in common, that means that a
 * path exists between each of these L1 words and any element in either of
 * their sets of connected words (from an L2 word to the directly-connected L1
 * word, then to the common L2 word, then to other L1 word). Therefore the
 * connections for both L1 words are the union of their original sets of
 * connections. So the algorithm boils down to: given some sets of integers,
 * merge any two sets that have at least one element in common. Repeat until no
 * further merging is possible. Plus some bookkeeping to keep track of the L1
 * words that go with the merged sets.
 *
 * Words that are unaligned, or that are explicitly aligned to the end position
 * in the other language (and to no other words), are not affected by this
 * operation.
 * 
 * @param sets1 word alignments in std WordAligner format. These are modified
 * by adding links for closure.
 * @param csets sets of L1 words that share the same set of L2 connections.
 * These are disjoint and cover all L1 words.
 */
void closure(vector< vector<Uint> >& sets1, vector< vector<Uint> >& csets)
{
   csets.clear();

   // a list of the L1 words that still need to be processed
   vector<Uint> todo(sets1.size());
   for (Uint i = 0; i < sets1.size(); ++i)
      todo[i] = i;

   QuickSet s1, s2;             // current L1, L2 merged sets

   // Do until all L1 words have been merged

   while (!todo.empty()) {

      s1.clear();
      s2.clear();
      Uint orig_s1_size;

      // Pick up the first set of L2 words that is left and grow it by
      // repeatedly sweeping over the other sets until no further merges are
      // possible.
      do {
         orig_s1_size = s1.size();
         for (Uint i = 0; i < todo.size();) {
            Uint w1 = todo[i];
            if (s1.empty() || intersectionNotNull(s2, sets1[w1])) {
               for (Uint j = 0; j < sets1[w1].size(); ++j)
                  s2.insert(sets1[w1][j]);
               s1.insert(w1);
               todo.erase(todo.begin()+i);
            } else
               ++i;
         }
      } while(s1.size() > orig_s1_size);

      // Remember current L1 cset, and put closure into all corresponding L2
      // sets. 

      csets.push_back(s1.contents());
      for (Uint i = 0; i < s1.contents().size(); ++i)
         sets1[s1.contents()[i]] = s2.contents();
   }

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
   const char* switches[] = {"v", "fin:", "fout:", "c", "t"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 4, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("fin", fin);
   arg_reader.testAndSet("fout", fout);
   arg_reader.testAndSet("fout", fout);
   arg_reader.testAndSet("c", do_closure);
   arg_reader.testAndSet("t", do_transpose);

   arg_reader.testAndSet(0, "text1", text1);
   arg_reader.testAndSet(1, "text2", text2);
   arg_reader.testAndSet(2, "al_in", al_in);
   arg_reader.testAndSet(3, "al_out", al_out);
}
