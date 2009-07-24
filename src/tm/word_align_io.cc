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

using namespace Portage;

/*------------------------------------------------------------------------------
  Writers
------------------------------------------------------------------------------*/

WordAlignmentWriter* WordAlignmentWriter::create(const string& format)
{
   WordAlignmentWriter* writer = NULL;
   if (format == "aachen")
      writer = new AachenWriter();
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
   else 
      error(ETFatal, "Unknown alignment format: %s", format.c_str());

   return writer;
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

/*------------------------------------------------------------------------------
  Readers
------------------------------------------------------------------------------*/

WordAlignmentReader* WordAlignmentReader::create(const string& format)
{
   WordAlignmentReader* writer = NULL;
   if (format == "hwa")
      writer = new HwaReader();
   else if (format == "green")
      writer = new GreenReader();
   else if (format == "sri")
      writer = new SRIReader();
   else 
      error(ETFatal, "Unknown alignment format: %s", format.c_str());

   return writer;
}

// format: index1 index2 (word1, word2)

istream& HwaReader::operator()(istream &in, 
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
         error(ETFatal, "length mismatch error for sentence aligment %d, line %d", sentence_id, line_num);
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

   return in;

}

istream& GreenReader::operator()(istream &in, 
                                 const vector<string>& toks1, const vector<string>& toks2,
                                 vector< vector<Uint> >& sets) 
{
   string line;
   vector<string> toks, subtoks;
   
   if (!getline(in, line))
      error(ETFatal, "aligment file too short");

   split(line, toks);
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
   return in;
}

istream& SRIReader::operator()(istream &in, 
                               const vector<string>& toks1, const vector<string>& toks2,
                               vector< vector<Uint> >& sets)
{
   string line;
   vector<string> toks, subtoks;

   sets.clear();
   sets.resize(toks1.size());

   if (!getline(in, line))
      error(ETFatal, "aligment file too short");
   split(line, toks);
   for (Uint i = 0; i < toks.size(); ++i) {
      if (splitZ(toks[i], subtoks, "-") != 2)
         error(ETFatal, "format error in SRI-style alignment: expecting tokens in format i-j");
      Uint l1 = conv<Uint>(subtoks[0]);
      Uint l2 = conv<Uint>(subtoks[1]);
      assert(l1 < toks1.size()); // lazy; should be error
      assert(l2 < toks2.size());
      sets[l1].push_back(l2);
   }

   return in;
}
