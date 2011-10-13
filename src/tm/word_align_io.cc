/**
 * @author George Foster
 * @file word_align_io.cc  Read and write word alignments in different formats
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <set>
#include "word_align_io.h"
#include "word_align.h"

using namespace Portage;

/*------------------------------------------------------------------------------
  Writers
------------------------------------------------------------------------------*/

WordAlignmentWriter* WordAlignmentWriter::create(const string& format)
{
   WordAlignmentWriter* writer = NULL;
   if (format == "aachen")
      writer = new AachenWriter();
   else if (format == "gale")
      writer = new GALEWriter();
   else if (format == "compact")
      writer = new CompactWriter();
   else if (format == "ugly")
      writer = new UglyWriter();
   else if (format == "matrix")
      writer = new MatrixWriter();
   else if (format == "hwa")
      writer = new HwaWriter();
   else if (format == "green")
      writer = new GreenWriter();
   else if (format == "sri")
      writer = new SRIWriter();
   else if (format == "uli")
      writer = new UliWriter();
   else 
      error(ETFatal, "Unknown alignment format: %s", format.c_str());

   assert(writer);

   return writer;
}

/*
 * example generated in test-suite/unit-testing/word_align_tool.
 * run make all, the examples will be generated into files align2.*
 */
string WordAlignmentWriter::help()
{
   return
"--- Word alignment formats ---\n\
\n\
Examples shown are for \"N a a b U\" and \"U A B B N\", with the a's aligned to A,\n\
b aligned to the B's, the N's NULL aligned, and the U's unaligned.\n\
\n\
Note: only green and gale are complete formats, i.e., formats which contain all\n\
the alignment information.\n\
\n\
aachen   Aachen style: one line for the sent ID followed by one line per link\n\
         incomplete: NULL-aligned and unaligned words are not distinguished.\n\
         e.g.: SENT: 0\n\
               S 1 1\n\
               S 2 1\n\
               S 3 2\n\
               S 3 3\n\
         \n\
gale     GALE (RWTH) alignment output style.\n\
         ID 0 # Source Sent # Raw Hyp # Postprocessed Hyp @ Alignment # Scores\n\
         e.g.: 0  0 # N a a b U # U A B B N # U A B B N @\n\
               A 0 5 A 1 1 A 2 1 A 3 2 A 3 3 A 5 4 # noscores\n\
         \n\
compact  <sent ID><tab><alignment>, where <alignment> is a semi-colon separated\n\
         list of comma-separated lists of indices.  The indices in the n-th\n\
         list indicate which lang2 words the n-th lang1 word is aligned to.\n\
         The lang2 word indices start at 1, contrary to all other formats.\n\
         incomplete: NULL-aligned and unaligned words are not distinguished.\n\
         e.g.: 0\t;2;2;3,4;;\n\
         \n\
ugly     List of aligned word pairs.\n\
         incomplete: identical words are not distinguished,\n\
         e.g.: a/A a/A b/B b/B\n\
               EXCLUDED: 0 / 4\n\
         \n\
matrix   Matrix format for easy visual inspection\n\
         incomplete: NULL-aligned and unaligned words are not distinguished.\n\
         e.g.: |U|A|B|B|N|\n\
               +-+-+-+-+-+\n\
               | | | | | |N\n\
               | |x| | | |a\n\
               | |x| | | |a\n\
               | | |x|x| |b\n\
               | | | | | |U\n\
               +-+-+-+-+-+\n\
         \n\
hwa      One sentence pair per file, in files named \"aligned.ID\"\n\
         incomplete: NULL-aligned and unaligned words are not distinguished.\n\
         e.g.: 1  1  (a, A)\n\
               2  1  (a, A)\n\
               3  2  (b, B)\n\
               3  3  (b, B)\n\
         \n\
green    Similar to compact, but complete, with 0 offset indices, no sentence\n\
         ID, and space separated.  Unaligned lang1 words are shown by \"-\",\n\
         NULL-aligned lang1 words are shown by the number of lang2 words,\n\
         unaligned lang2 words are implicit, NULL-aligned lang2 words are\n\
         listed in the optional last element of the top-level list.\n\
         e.g.: 5 1 1 2,3 - 4\n\
         \n\
sri      List of lang1-lang2 index pairs representing the alignment links.\n\
         incomplete: NULL-aligned and unaligned words are not distinguished.\n\
         e.g.: 1-1 2-1 3-2 3-3\n\
\n\
uli      Format for Uli's YAWAT tool.\n\
         incomplete: represents the transitive closure of the alignment.\n\
         incomplete: NULL-aligned and unaligned words are not distinguished.\n\
         e.g.: 1 1,2:1:unspec 3:2,3:unspec\n\
\n\
";

}

ostream& UglyWriter::operator()(ostream &out, 
                                 const vector<string>& toks1, const vector<string>& toks2,
                                 const vector< vector<Uint> >& sets) {
   bool exclusions_exist = false;
   for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
	 if (sets[i][j] < toks2.size())
	    out << toks1[i] << "/" << toks2[sets[i][j]] << " ";
	 else
	    exclusions_exist = true;
   out << endl;

   if (exclusions_exist || sets.size() > toks1.size()) {
      out << "EXCLUDED: ";
      for (Uint i = 0; i < toks1.size(); ++i)
	 for (Uint j = 0; j < sets[i].size(); ++j)
	    if (sets[i][j] == toks2.size())
	       out << i << " ";
      out << "/ ";
      if (sets.size() > toks1.size())
	 for (Uint i = 0; i < sets.back().size(); ++i)
	    out << sets.back()[i] << " ";
      out << endl;
   }
   return out;
}


ostream& AachenWriter::operator()(ostream &out, 
                                   const vector<string>& toks1, const vector<string>& toks2,
                                   const vector< vector<Uint> >& sets) {
   out << "SENT: " << sentence_id++ << endl;
   
   for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
         if (sets[i][j] < toks2.size())
            out << "S " << i << ' ' << sets[i][j] << endl;
   out << endl;
   return out;
}

// ID 0 # Source Sentence # Raw Hypothesis # Postprocessed Hypothesis @ Alignment # Scores
ostream& GALEWriter::operator()(ostream &out, 
                                const vector<string>& toks1, const vector<string>& toks2,
                                const vector< vector<Uint> >& sets) {
   const vector<string>& pptoks2(postproc_toks2 ? *postproc_toks2 : toks2);

   out << sentence_id << "  0 #";

   for (Uint i = 0; i < toks1.size(); ++i)
      out << " " << toks1[i];
   out << " #";
   for (Uint i = 0; i < toks2.size(); ++i)
      out << " " << toks2[i];
   out << " #";
   for (Uint i = 0; i < pptoks2.size(); ++i)
      out << " " << pptoks2[i];
   out << " @";
   for (Uint i = 0; i < sets.size(); ++i) {
      for (Uint j = 0; j < sets[i].size(); ++j)
         out << " A " << i << " " << sets[i][j];
   }
   out << " # noscores" << endl;
   
   return out;
}

ostream& CompactWriter::operator()(ostream &out, 
                                    const vector<string>& toks1, const vector<string>& toks2,
                                    const vector< vector<Uint> >& sets) {
   out << sentence_id++ << '\t';
   for (Uint i = 0; i < toks1.size(); ++i) {
      Uint count = 0;
      for (Uint j = 0; j < sets[i].size(); ++j)
         if (sets[i][j] < toks2.size()) {
            if (count++) out << ',';
            out << sets[i][j]+1;
         }
      out << ';';
   }
   out << endl;
   return out;
}

ostream& HwaWriter::operator()(ostream &out, 
                                const vector<string>& toks1, const vector<string>& toks2,
                                const vector< vector<Uint> >& sets) {
   ostringstream fname;
   fname << "aligned." << ++sentence_id;
   oSafeMagicStream fout(fname.str());
   
   for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
         if (sets[i][j] < toks2.size())
            fout << i << "  " << sets[i][j] 
                 << "  (" << toks1[i] << ", " << toks2[sets[i][j]] << ")" 
                 << endl;
   
   fout.close();
   return out;
}

ostream& MatrixWriter::operator()(ostream &out, 
                                   const vector<string>& toks1, const vector<string>& toks2,
                                   const vector< vector<Uint> >& sets) {
   // Write out column names
    Uint more = toks2.size(); // any non-zero value would do actually
    for (Uint k = 0; more > 0; ++k) {
       more = 0;
       for (Uint j = 0; j < toks2.size(); ++j) {
          out << '|' << ((k < toks2[j].size()) ? toks2[j][k] : ' ');
          if (k+1 < toks2[j].size()) ++more;
       }
       out << '|' << endl;
    }
    
    //top ruler
    for (Uint j = 0; j < toks2.size(); ++j) 
       out << "+-";
    out << "+" << endl;
    
    // write out rows
    vector<char> xs(toks2.size());
    for (Uint i = 0; i < toks1.size(); ++i) {
       xs.assign(toks2.size(), ' ');
       for (Uint k = 0; k < sets[i].size(); ++k)
          if (sets[i][k] < toks2.size())
             xs[sets[i][k]] = 'x';
       for (Uint j = 0; j < toks2.size(); ++j)
          out << '|' << xs[j];
       out << "|" << toks1[i] << endl;
    }
    
    // bottom ruler
    for (Uint j = 0; j < toks2.size(); ++j) 
       out << "+-";
    out << "+" << endl << endl;
    return out;
}

ostream& GreenWriter::operator()(ostream &out, 
                                 const vector<string>& toks1, const vector<string>& toks2,
                                 const vector< vector<Uint> >& sets) 
{
   for (Uint i = 0; i < sets.size(); ++i) {
      if (sets[i].empty())
         out << '-';
      for (Uint j = 0; j < sets[i].size(); ++j) {
         out << sets[i][j];
         if (j+1 < sets[i].size()) out << ',';
      }
      if (i+1 < sets.size())
         out << ' ';
   }
   out << endl;
   return out;
}

void GreenWriter::write_partial_alignment(string& out,
      const vector<string>& toks1, Uint beg1, Uint end1,
      const vector<string>& toks2, Uint beg2, Uint end2,
      const vector< vector<Uint> >& sets, char sep)
{
   assert(beg1 <= toks1.size());
   assert(end1 <= toks1.size());
   assert(beg2 <= toks2.size());
   assert(end2 <= toks2.size());
   assert((sets.size() == toks1.size()) || (sets.size() == toks1.size() + 1));

   ostringstream ss;

   for ( Uint i = beg1; i < end1; ++i ) {
      bool empty(true);
      for ( Uint j = 0; j < sets[i].size(); ++j ) {
         Uint jj = sets[i][j];
         if ( beg2 <= jj && jj < end2 ) {
            if (empty)
               empty = false;
            else
               ss << ',';
            ss << jj - beg2;
         } else if ( jj == toks2.size() ) {
            if ( !empty ) error(ETFatal, "NULL and regular alignment for same word.");
            empty = false;
            ss << (end2 - beg2);
         }
      }
      if ( empty )
         ss << '-';
      if ( i+1 < end1 )
         ss << sep;
   }
   if (sets.size() > toks1.size()) {
      Uint i = toks1.size();
      bool empty(true);
      for ( Uint j = 0; j < sets[i].size(); ++j ) {
         Uint jj = sets[i][j];
         if ( beg2 <= jj && jj < end2 ) {
            if (empty) {
               empty = false;
               if ( beg1 < end1 )
                  ss << sep;
            } else
               ss << ',';
            ss << jj - beg2;
         } else if ( jj == toks2.size() ) {
            error(ETFatal, "NULL - NULL alignment is not meaningful.");
         }
      }
   }

   out = ss.str();
} // GreenWriter::write_partial_alignment()

void GreenWriter::reverse_alignment(string& out, const char* green_alignment,
                                    Uint toks1_len, Uint toks2_len, char sep)
{
   if ( 1 )
   // This variant is ugly, but 62% faster (i.e., it runs in 38% of the time
   // the alternative takes)
   // Optimized by parsing into flat auto arrays with offset lists, instead of
   // vectors of vectors.
   {
      // parse the alignment
      Uint len = strlen(green_alignment);
      // compact structure to hold alignments
      // al[j] has the tok2 offsets for alignment links
      Uint al[len];
      // al_start[i] says where the alignments for toks1[i] start
      Uint al_start[toks1_len+2];
      // rev_al_count[j] says how many toks1 elements were aligned to j
      Uint rev_al_count[toks2_len+1];
      for ( Uint j = 0; j <= toks2_len; ++j ) rev_al_count[j] = 0;
      char alignment_copy[len+1];
      strcpy(alignment_copy, green_alignment);
      char seps1[2] = {sep, '\0'};
      char seps2[] = ",";
      char *saveptr1, *saveptr2;
      char* num1 = strtok_r(alignment_copy, seps1, &saveptr1);
      Uint next_al_index(0);
      Uint next_toks1_index(0);
      while (num1 != NULL) {
         if ( next_toks1_index > toks1_len )
            error(ETFatal, "Alignment has too many fields for %u source tokens: %s",
                  toks1_len, green_alignment);
         al_start[next_toks1_index++] = next_al_index;
         if (num1[0] == '-' && num1[1] == '\0') {
            ; // no alignments here, just skip
         } else {
            char* num2 = strtok_r(num1, seps2, &saveptr2);
            while (num2 != NULL) {
               Uint j = conv<Uint>(num2);
               if ( j > toks2_len )
                  error(ETFatal, "Alignment value out of range given %u target tokens: %s",
                        j, green_alignment);
               al[next_al_index++] = j;
               ++rev_al_count[j];
               num2 = strtok_r(NULL, seps2, &saveptr2);
            }
         }
         num1 = strtok_r(NULL, seps1, &saveptr1);
      }
      if ( next_toks1_index < toks1_len )
         error(ETFatal, "Alignment has too few fields for %u source tokens: %s",
               toks1_len, green_alignment);
      assert(next_al_index <= len);
      al_start[next_toks1_index++] = next_al_index;
      if ( next_toks1_index <= toks1_len + 1 )
         al_start[next_toks1_index++] = next_al_index;
      const Uint toks1_with_null_len = toks1_len +
         (al_start[toks1_len] == al_start[toks1_len+1] ? 0 : 1);
      assert(al_start[0] == 0);

      // reverse the alignment
      Uint rev_al_next[toks2_len+1];
      rev_al_next[0] = 0;
      Uint rev_al_start[toks2_len+2];
      rev_al_start[0] = 0;
      for ( Uint j = 0; j < toks2_len; ++j )
         rev_al_start[j+1] = rev_al_next[j+1] = rev_al_next[j] + rev_al_count[j];
      rev_al_start[toks2_len+1] = rev_al_next[toks2_len] + rev_al_count[toks2_len];
      assert(rev_al_start[toks2_len+1] == next_al_index);
      Uint rev_al[next_al_index];
      Uint j = 0;
      for ( Uint i = 0; i < toks1_with_null_len; ++i )
         for ( ; j < al_start[i+1]; ++j )
            rev_al[rev_al_next[al[j]]++] = i;
      assert(j == next_al_index);

      // display the alignment
      ostringstream oss;
      const Uint toks2_with_null_len = toks2_len +
         (rev_al_start[toks2_len] == rev_al_start[toks2_len+1] ? 0 : 1);
      for ( Uint i = 0; i < toks2_with_null_len; ++i ) {
         if ( rev_al_start[i] == rev_al_start[i+1] )
            oss << '-';
         for (Uint j = rev_al_start[i]; j < rev_al_start[i+1]; ++j) {
            oss << rev_al[j];
            if (j+1 < rev_al_start[i+1]) oss << ',';
         }
         if (i+1 < toks2_with_null_len)
            oss << sep;
      }
      out = oss.str();

   }
   else
   // Simpler but much slower because we parse into vectors of vectors.
   {

      GreenReader reader;
      vector<vector<Uint> > sets;
      vector<string> toks1(toks1_len);
      vector<string> toks2(toks2_len);
      istringstream iss;
      if ( sep != ' ' ) {
         Uint len = strlen(green_alignment);
         char green2[len+1];
         for ( Uint i = 0; i < len; ++i ) {
            if ( green_alignment[i] == sep )
               green2[i] = ' ';
            else
               green2[i] = green_alignment[i];
         }
         green2[len] = '\0';
         iss.str(green2);
      } else {
         iss.str(green_alignment);
      }
      reader(iss, toks1, toks2, sets);

      //this->operator()(cerr, toks1, toks2, sets);
      
      vector<vector<Uint> > reverse_sets (toks2_len+1);
      for ( Uint i = 0; i < sets.size(); ++i ) {
         for ( Uint j = 0; j < sets[i].size(); ++j ) {
            Uint jj = sets[i][j];
            if ( jj <= toks2_len )
               reverse_sets[jj].push_back(i);
            else
               error(ETFatal, "Bad alignment string %s with lengths %u and %u",
                     green_alignment, toks1_len, toks2_len);
         }
      }
      // If nothing is NULL aligned, remove the NULL alignment list.
      if ( reverse_sets.back().empty() )
         reverse_sets.pop_back();

      ostringstream oss;
      GreenWriter().operator()(oss, toks2, toks1, reverse_sets);
      out = oss.str();
      // strip the newline
      if ( !out.empty() )
         out.resize(out.length() - 1);
      // change ' ' for the chosen separator
      if ( sep != ' ' )
         for ( Uint i = 0; i < out.size(); ++i )
            if ( out[i] == ' ' )
               out[i] = sep;

   }
}


ostream& SRIWriter::operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets) 
{
   bool first = true;
   for (Uint i = 0; i < toks1.size(); ++i)
      for (Uint j = 0; j < sets[i].size(); ++j)
         if (sets[i][j] < toks2.size()) {
            if (!first) out << ' ';
            else first = false;
            out << i << '-' << sets[i][j];
         }
   out << endl;
   return out;
}

ostream& UliWriter::operator()(ostream &out, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               const vector< vector<Uint> >& sets)
{
   msets = sets;
   WordAligner::close(msets, csets);

   out << ++sentence_id << ' ';
   for (Uint i = 0; i < csets.size(); ++i) {
      if (csets[i].size() == 0 || 
          csets[i][0] == toks1.size() || // tok1 side is null
          msets[csets[i][0]].size() == 0 ||
          msets[csets[i][0]][0] == toks2.size()) // tok2 side is null
         continue;
      for (Uint j = 0; j < csets[i].size(); ++j)
         out << csets[i][j] << (j+1 < csets[i].size() ? "," : "");
      out << ":";
      vector<Uint>& mset = msets[csets[i][0]];
      for (Uint j = 0; j < mset.size(); ++j)
         out << mset[j] << (j+1 < mset.size() ? "," : "");
      out << ":unspec ";
   }
   out << endl;
   return out;
}

/*------------------------------------------------------------------------------
  Readers
------------------------------------------------------------------------------*/

WordAlignmentReader* WordAlignmentReader::create(const string& format)
{
   WordAlignmentReader* reader = NULL;
   if (format == "hwa")
      reader = new HwaReader();
   else if (format == "green")
      reader = new GreenReader();
   else if (format == "sri")
      reader = new SRIReader();
   else 
      error(ETFatal, "Unknown alignment format: %s", format.c_str());

   assert(reader);

   return reader;
}

// format: index1 index2 (word1, word2)

bool HwaReader::operator()(istream &in, 
                           const vector<string>& toks1, const vector<string>& toks2,
                           vector< vector<Uint> >& sets)
{
   ostringstream fname;
   fname << "aligned-in." << ++sentence_id;
   iSafeMagicStream fin(fname.str());

   sets.resize(toks1.size());
   for (Uint i = 0; i < sets.size(); ++i)
      sets[i].clear();

   Uint line_num = 0;
   string line;
   vector<string> toks;
   while (getline(fin, line)) {
      ++line_num;
      Uint ntoks = splitZ(line, toks, "\n\r ");
      if (ntoks == 0) {
         error(ETWarn, "skipping blank line in sentence alignment %d, line %d", 
               sentence_id, line_num);
         continue;
      }
      if (ntoks != 4 || toks[2].length() < 3 || toks[3].length() < 2)
         error(ETFatal, "bad format for sentence alignment %d, line %d", sentence_id, line_num);
      Uint index1 = conv<Uint>(toks[0]);
      Uint index2 = conv<Uint>(toks[1]);
      string tok1 = toks[2].substr(1, toks[2].length()-2); // (word1, -> word1
      string tok2 = toks[3].substr(0, toks[3].length()-1); // word2)  -> word2

      if (index1 >= toks1.size() || index2 >= toks2.size())
         error(ETFatal, "length mismatch error for sentence alignment %d, line %d", sentence_id, line_num);
      if (tok1 != toks1[index1])
         error(ETWarn, "L1 token mismatch error for sentence alignment %d, line %d: %s vs %s", 
               sentence_id, line_num, tok1.c_str(), toks1[index1].c_str());
      if (tok2 != toks2[index2])
         error(ETWarn, "L2 token mismatch error for sentence alignment %d, line %d: %s vs %s", 
               sentence_id, line_num, tok2.c_str(), toks2[index2].c_str());
      
      sets[index1].push_back(index2);
   }

   // enforce semantics of Hwa's alignment interface: words with no links are
   // really untranslated, not just lacking a direct translation:

   set<Uint> cover2;

   for (Uint i = 0; i < toks1.size(); ++i)
      if (sets[i].empty())        // no connection for toks1[i], so connect it to NULL
         sets[i].push_back(toks2.size());
      else                      // record connections for toks1[i] in cover2
         for (Uint j = 0; j < sets[i].size(); ++j)
            cover2.insert(sets[i][j]);

   // make explicit NULL connections for unconnected toks2 words
   for (Uint i = 0; i < toks2.size(); ++i)
      if (cover2.find(i) == cover2.end()) {
         if (sets.size() == toks1.size())
            sets.push_back(vector<Uint>());
         sets[toks1.size()].push_back(i);
      }

   return true;

}

bool GreenReader::operator()(istream &in, 
                             const vector<string>& toks1, const vector<string>& toks2,
                             vector< vector<Uint> >& sets) 
{
   string line;
   if (!getline(in, line))
      return false;
      //error(ETFatal, "alignment file too short");
   operator()(line, sets);
   return true;
}

void GreenReader::operator()(const string& line, vector< vector<Uint> >& sets) 
{
   vector<string> toks, subtoks;
   const char seps[2] = {sep, '\0'};
   split(line, toks, seps);
   sets.resize(toks.size());

   for (Uint i = 0; i < sets.size(); ++i) {
      sets[i].clear();
      if (toks[i] != "-") {
         splitZ(toks[i], subtoks, ",");
         sets[i].resize(subtoks.size());
         for (Uint j = 0; j < subtoks.size(); ++j)
            sets[i][j] = conv<Uint>(subtoks[j]);
      }
   }   
}

bool SRIReader::operator()(istream &in, 
                           const vector<string>& toks1, const vector<string>& toks2,
                           vector< vector<Uint> >& sets)
{
   string line;
   vector<string> toks, subtoks;

   sets.clear();
   sets.resize(toks1.size());

   if (!getline(in, line))
      return false;
      //error(ETFatal, "alignment file too short");

   split(line, toks);
   for (Uint i = 0; i < toks.size(); ++i) {
      if (splitZ(toks[i], subtoks, "-") != 2)
         error(ETFatal, "format error in SRI-style alignment: expecting tokens in format i-j.");
      Uint l1 = conv<Uint>(subtoks[0]);
      Uint l2 = conv<Uint>(subtoks[1]);
      if ( l1 >= toks1.size() )
         error(ETFatal, "format error in SRI-style alignment: l1 position past end of sentence.");
      if ( l2 >= toks2.size() )
         error(ETFatal, "format error in SRI-style alignment: l2 position past end of sentence.");
      sets[l1].push_back(l2);
   }

   return true;
}
