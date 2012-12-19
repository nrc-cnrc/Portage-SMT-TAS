/**
 * @author George Foster
 * @file markup_canoe_output.cc
 * @brief Add source-side bracketing markup to canoe output, in the right places.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <limits>
#include <iostream>
#include <fstream>
#include "file_utils.h"
#include "arg_reader.h"
#include "parse_xmlish_markup.h"
#include "parse_pal.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
markup_canoe_output [options] msrc out pal\n\
\n\
Add XML-style tags in source file MSRC to canoe output OUT, using phrase-\n\
alignment file PAL to determine proper placement. MSRC is a version of the\n\
source file SRC fed to canoe, in which optional tags have been added, in the\n\
form:\n\
\n\
   <tag...>SRC-PHRASE</tag>\n\
\n\
For each occurrence of such a tag pair, this program will find a target phrase\n\
TGT-PHRASE in OUT that it deems to be the best translation of SRC-PHRASE, and\n\
will replace that occurrence of TGT-PHRASE with <tag...>TGT-PHRASE</tag>.  Any\n\
tags that are empty or that do not have a matching tag on the current line are\n\
ignored. Any tags that are specified by the -n flag below are treated as\n\
ordinary tokens.  Note that normal canoe markup, eg <rule ...>SRC-PHRASE</rule>,\n\
will be considered to be tags, and should be removed from MSRC if this is not\n\
desired. Output is written to stdout.\n\
\n\
NB: The PAL file can be generated by calling canoe like this:\n\
   canoe -f canoe.ini -palign < SRC | nbest2rescore.pl -canoe -palout=PAL > OUT\n\
\n\
With -xtags, empty tags are transferred too, and paired tags (e.g. bpt, ept)\n\
are swapped when necessary to preserve correct tag ordering.\n\
\n\
Options:\n\
  -v    Write progress reports to cerr.\n\
  -a    Do word alignment to determine target span even when phrase boundaries\n\
        line up with tag boundaries [word-align only when boundaries don't align]\n\
  -n t  Treat all tags of the form <T...> or </T> as ordinary tokens. This option\n\
        may be repeated to specify multiple tags. Warning: the tokenization \n\
        implied by this option must match that described by the PAL argument -\n\
        beware of problems caused by tags that consist of multiple elements or\n\
        ones that abut normal tokens.\n\
  -e    Don't escape XML reserved characters <& in output. [Escape these chars\n\
        except when in added tags or those specified by -n.]\n\
  -xtags Handle formatting XML tags from TMX and SDLXLIFF file formats [don't]\n\
";

// globals

static bool verbose = false;
static bool align_boundary_phrases = false;
static bool no_escaping = false;
static bool xtags = false;
static string msrcfile;
static string outfile;
static string palfile;
static vector<string> tags_to_ignore;
static void getArgs(int argc, char* argv[]);

// represent an XML element: pair of tags, with token positions, possibly
// identical (if element is empty)

struct Elem {

   string name;                 // element name
   string lstring;              // left tag string, eg '<name atr="xxx">'
   string rstring;              // right tag string, eg '</name>'
   Uint btok;                   // index of 1st src tok to the right of left tag
   Uint etok;                   // index of 1st src tok to the right of right tag
   bool complete;               // true if right tag seen (& etok is valid)
   bool lwsl;                   // true if whitespace to left of left tag
   bool lwsr;                   // true if whitespace to right of left tag
   bool rwsl;                   // true if whitespace to left of right tag
   bool rwsr;                   // true if whitespace to right of right tag

   bool palign;                 // elem is aligned w/ src-side phrase boundaries
   bool contig;                 // elem's tgt phrases are contiguous
   Uint btok_tgt;               // index of 1st tgt tok to the right of left tag
   Uint etok_tgt;               // index of 1st tgt tok to the right of right tag

   // create, from left tag positions within line
   Elem(const string& line, Uint beg, Uint end, Uint btok, bool wsl, bool wsr) :
      name(getName(line, beg, end)),
      lstring(line.substr(beg, end-beg)), rstring(""),
      btok(btok), etok(0), complete(false),
      lwsl(wsl), lwsr(wsr), rwsl(true), rwsr(true),
      btok_tgt(0), etok_tgt(0)
   {}

   static bool isLeft(const string& line, Uint beg, Uint end) {
      return line[beg+1] != '/';
   }
   
   static bool isRight(const string& line, Uint beg, Uint end) {
      return line[beg+1] == '/' || line[end-2] == '/';
   }

   static string getName(const string& line, Uint beg, Uint end) {
      Uint s = line[beg+1] == '/' ? beg+2 : beg+1;
      Uint e = line.find_first_of(" >\t\n", s);
      assert (e != string::npos && e < end);
      return line.substr(s, e-s);
   }
   
   void dump() {
      cerr << name << ": " << lstring << " " << rstring << " " << complete
           << " src:(" << btok << "," << etok << ")"
           << " tgt:(" << btok_tgt << "," << etok_tgt << ")"
           << " whitespace: " << lwsl << lwsr << rwsl << rwsr << endl;
   }

   bool empty() {return btok == etok;}

   bool isIntraTokenLeft() {
      return !lwsl && !lwsr;
   }

   bool isIntraTokenRight() {
      return !rwsl && !rwsr;
   }

   // update, from right tag positions within line
   void update(const string& line, Uint beg, Uint end, Uint etok_tag,
               bool wsl, bool wsr)
   {
      if (line[beg+1] == '/')
         rstring = line.substr(beg, end-beg);
      etok = etok_tag;
      complete = true;
      rwsl = wsl;
      rwsr = wsr;
   }
};

// Initial stab at a bilingual dictionary, customized with a few ad hoc en/fr
// entries. TODO: normalize case before comparing, allow spec of coding scheme,
// allow dict to be read in from file.

struct Dict {

   typedef map<string,Uint> Map;
   
   Map dict;

   Dict() {}

   // Initialize with given paired entries (order doesn't matter).

   Dict(Uint num_entries, pair<string,string> entries[]) {
      for (Uint i = 0; i < num_entries; ++i) {
         dict[entries[i].first] = i;
         dict[entries[i].second] = i;
      }
   }

   // Return true if w1 is a translation of w2 (order doesn't matter).

   bool match(const string& w1, const string& w2) const {
      Map::const_iterator p1 = dict.find(w1);
      Map::const_iterator p2 = dict.find(w2);
      return p1 != dict.end() && p2 != dict.end() && p1->second == p2->second;
   }
};

// Estimate the score for a word pair within a phrase pair of length (I>0, J>0)
// by comparing its position to a diagonal line that connects (0,0) to (I,J).
// Calculate horizontal and vertical distances to the diagonal, sum them, add
// one, then return their inverse, so that higher scores are better. Scores are
// in (0,1].

double positionScore(Uint i, Uint j, Uint I, Uint J)
{
   double slope = J/(double)I;
   double x = double(i) + 0.5;  // word spans [i,i+1], so represent with i+0.5
   double y = double(j) + 0.5;  // word spans [j,j+1], so represent with j+0.5
   double delta_y = y - x * slope;
   double delta_x = x - x / slope;
   return 1.0 / (delta_x * delta_x + delta_y * delta_y + 1.0);
}

/**
 * Heuristically align words within an aligned phrase pair. Very simple 1-way
 * alignment.  
 * @param dict word-pair dictionary
 * @param anti_dict word-pair anti-dictionary
 * @param pp phrase pair to align
 * @param src_toks source sentence that contains source phrase
 * @param tgt_toks target sentence that contains target phrase
 * @param src_al alignment: for each i in [0,S-1], where S is the number of
 * words in the source phrase, src_al[i] will be set to the index of an aligned
 * word in the target phrase. NB: indexes for src_al are relative to pp, not
 * the containing sentence.
 */
void alignPhrasePair(const Dict& dict, const Dict& anti_dict, 
                     const PhrasePair& pp, 
                     const vector<string>& src_toks, 
                     const vector<string>& tgt_toks,
                     vector<Uint>& src_al) 
{
   static char punc_num[] = "0123456789=[];,./!@#$%^&*()_+{}:<>?";
   
   Uint slen = pp.src_pos.second - pp.src_pos.first;
   Uint tlen = pp.tgt_pos.second - pp.tgt_pos.first;

   src_al.resize(slen);
   vector<double> src_scores(slen, 0.0);

   for (Uint i = 0; i < slen; ++i) {
      for (Uint j = 0; j < tlen; ++j) {
         const string& stok = src_toks[i+pp.src_pos.first];
         const string& ttok = tgt_toks[j+pp.tgt_pos.first];
         double score = positionScore(i, j, slen, tlen);
         if ((
              longestPrefix(stok.c_str(), ttok.c_str()) >= 4 ||
              (stok == ttok && stok.find_first_of(punc_num) != string::npos) ||
              dict.match(stok, ttok)) 
             && !anti_dict.match(stok, ttok))
            score += 1;  // cognate match
         if(verbose)
             cerr << "alignPhrasePair: stok: " << stok << " ttok: " << ttok << " score: " << score << " src_scores[" << i << "]=" << src_scores[i] << endl;
         if (score > src_scores[i]) {
            if(verbose)
                cerr << "alignPhrasePair: setting src_all[" << i << "]=" << j << endl;
            src_scores[i] = score;
            src_al[i] = j;
          }
      }
      if(verbose)
          cerr << "alignPhrasePair: src_all[" << i << "]=" << src_al[i] << endl;
   }
}

/**
 * Find the target span that corresponds to a given span of a source alignment.
 * @param src_al source alignment, as returned by alignPhrasePair
 * @param pp phrase pair that src_al pertains to 
 * @param beg index of beg src word in span, in range of pp.src_pos
 * @param beg index of end+1 src word in span, in range of pp.src_pos
 * @return [beg,end) of target span aligned with given source span, in range of
 * pp.tgt_pos
 */
pair<Uint,Uint> targetSpan(const vector<Uint>& src_al, const PhrasePair& pp,
                           Uint beg, Uint end)
{
   if (!align_boundary_phrases && 
       beg == pp.src_pos.first && end == pp.src_pos.second)
      return make_pair(pp.tgt_pos.first, pp.tgt_pos.second);

   // point tag
   if (beg == end)
      return make_pair(pp.tgt_pos.first + src_al[beg-pp.src_pos.first],
                       pp.tgt_pos.first + src_al[beg-pp.src_pos.first]);

   pair<Uint,Uint> ret(make_pair(numeric_limits<Uint>::max(), 0));
   for (Uint i = beg-pp.src_pos.first; i < end-pp.src_pos.first; ++i) {
      if (src_al[i] < ret.first) ret.first = src_al[i];
      if (src_al[i] > ret.second) ret.second = src_al[i];
   }

   ret.first += pp.tgt_pos.first;
   ret.second += pp.tgt_pos.first + 1;

   return ret;
}

/**
 * Return true if this tag should be ignored.
 */
inline bool ignoreTag(const string &tag_name)
{
   return find(tags_to_ignore.begin(), tags_to_ignore.end(), tag_name) !=
          tags_to_ignore.end();
}

/**
 * Convert a target token into an XML-escaped version.
 */
string escape(string& tok)
{
   if (no_escaping) 
      return tok;
   if (tok.size() > 0 && tok[0] == '<') {
      string::size_type end = tok.find_first_of(" \t/>");
      if (end == string::npos) end = tok.size();
      if (ignoreTag(tok.substr(1, end-1)))
         return tok;
   }

   string ret;
   for (Uint i = 0; i < tok.size(); ++i)
      if (tok[i] == '<')
         ret += "&lt;";
      else if (tok[i] == '&')
         ret += "&amp;";
      else
         ret += tok[i];

   return ret;
}

/**
 * Split a string into tokens appending to previously extracted toks,
 * merging the first one with the previous last token if indicated.
 */
void splitAndMerge(const string &s, vector<string> &toks, bool merge_needed)
{
   Uint mtok = merge_needed ? toks.size() : 0;
   split(s, toks);
   if (mtok && toks.size() > mtok) {
      toks[mtok-1].append(toks[mtok]);
      toks.erase(toks.begin() + mtok);
   }
}


// main

int main(int argc, char* argv[])
{
   printCopyright(2010, "markup_canoe_output");

   getArgs(argc, argv);

   iSafeMagicStream msrc(msrcfile);
   iSafeMagicStream out(outfile);
   iSafeMagicStream pal(palfile);

   // set up ad hoc dictionaries

   pair<string,string> dict_entries[] = {
      make_pair("premier", "prime"), make_pair("defence", "d\303\251fense"),
   };
   Dict dict(ARRAY_SIZE(dict_entries), dict_entries);

   pair<string,string> anti_entries[] = {
      make_pair("minist\303\250re", "minister")
   };
   Dict anti_dict(ARRAY_SIZE(anti_entries), anti_entries);

   // read all three input files line-synchronously

   vector<Elem> elems;
   vector<string> src_toks;
   vector<string> tgt_toks;
   vector<Uint> src_al;
   vector<PhrasePair> phrase_pairs;
   vector<PhrasePair> pp_span;
   string line;
   Uint lineno = 0;
   Uint num_unmatched_tags = 0;
   Uint num_empty_elems = 0;
   Uint num_elems = 0;
   Uint num_elems_aligned = 0;
   Uint num_elems_contig = 0;
   bool is_intra_tok_tag = false;
   const string whitespace(" \t\n");

   while (getline(msrc, line)) {

      ++lineno;
      if (verbose)
         cerr << "line " << lineno << endl;

      // tokenize source line and record elements

      src_toks.clear();
      elems.clear();
      string::size_type p = 0, beg = 0, end = 0;
      bool wsl = true, wsr = true;
      while (findXMLishTag(line, p, beg, end)) {
         // Determine wsl and wsr for a sequence of tags when the first tag
         // in the sequence is encountered.
         if (beg != p || beg == 0) {
            wsl = beg > 0 ? (whitespace.find(line[beg-1]) != string::npos) : true;
            string::size_type p2 = end, b2, e2;
            while (findXMLishTag(line, p2, b2, e2)) {
               if (b2 != p2) break;
               if( ignoreTag(Elem::getName(line, beg, end))) break;
               p2 = e2;
            }
            wsr = whitespace.find(line[p2]) != string::npos;
         }
         string name = Elem::getName(line, beg, end);
         if (ignoreTag(name)) {
            splitAndMerge(line.substr(p, end-p), src_toks, is_intra_tok_tag);
            p = end;
            continue;
         }
         splitAndMerge(line.substr(p, beg-p), src_toks, is_intra_tok_tag);
         if (Elem::isLeft(line, beg, end)) {
            Uint btok = (!wsl && !wsr) ? src_toks.size()-1 : src_toks.size();
            elems.push_back(Elem(line, beg, end, btok, wsl, wsr));
            is_intra_tok_tag = elems.back().isIntraTokenLeft();
         }
         if (Elem::isRight(line, beg, end)) { // find a match
            Uint i = elems.size();
            for (; i > 0; --i) {
               if (!elems[i-1].complete && elems[i-1].name == name) {
                  Uint etok = (!wsl && !wsr) ? src_toks.size()-1 : src_toks.size();
                  elems[i-1].update(line, beg, end, etok, wsl, wsr);
                  is_intra_tok_tag = elems[i-1].isIntraTokenRight();
                  break;
               }
            }
            if (i == 0) {
               string tag = line.substr(beg, end-beg);
               if (verbose)
                  error(ETWarn, "line %d: %s has no matching left tag - ignoring",
                        lineno, tag.c_str());
               ++num_unmatched_tags;
            }
         }
         p = end;
      }
      splitAndMerge(line.substr(p), src_toks, is_intra_tok_tag); // add remaining tokens

      // read and tokenize tgt line

      if (!getline(out, line))
         error(ETFatal, "%s too short", outfile.c_str());
      splitZ(line, tgt_toks);

      // read, parse, and check pal line

      if (!getline(pal, line))
         error(ETFatal, "%s too short", palfile.c_str());
      if (!parsePhraseAlign(line, phrase_pairs))
         error(ETFatal, "can't parse line %d from %s: \n%s\n", 
               lineno, palfile.c_str(), line.c_str());
      sortByTarget(phrase_pairs);
      if (!targetContig(phrase_pairs))
         error(ETFatal, "line %d: target phrases not contiguous in %s", 
               lineno, palfile.c_str());
      if (phrase_pairs.size() && phrase_pairs.back().tgt_pos.second != tgt_toks.size())
         error(ETFatal, "line %d: %s specifies wrong number of target tokens", 
               lineno, palfile.c_str());
      sortBySource(phrase_pairs); // code below assumes this ppty
      if (!sourceContig(phrase_pairs))
         error(ETFatal, "line %d: source phrases not contiguous in %s", 
               lineno, palfile.c_str());
      if (phrase_pairs.size() && phrase_pairs.back().src_pos.second != src_toks.size())
         error(ETFatal, "line %d: %s specifies wrong number of source tokens", 
               lineno, palfile.c_str());

      // assign target positions to elements

      for (Uint i = 0; i < elems.size(); ++i) {
         if (verbose) elems[i].dump();
         if (!elems[i].complete) {
            error(ETWarn, "line %d: %s has no matching right tag - ignoring",
                  lineno, elems[i].lstring.c_str());
            ++num_unmatched_tags;
            continue;
         }
         if (!xtags && elems[i].empty()) {
            error(ETWarn, "line %d: %s is empty - ignoring", 
               lineno, elems[i].lstring.c_str());
            ++num_empty_elems;
            continue;
         }
         ++num_elems;

         // find bracketing phrase pairs

         Uint lp = 0;  // leftmost phrase that overlaps with elem
         for (; lp < phrase_pairs.size(); ++lp)
            if (phrase_pairs[lp].src_pos.second > elems[i].btok)
               break;
         Uint rp = phrase_pairs.size(); // rightmost phrase that overlaps w/ elem
         if (elems[i].btok == elems[i].etok)
            rp = lp;       // point tag
         else {
            for (; rp > 0; --rp)
               if (phrase_pairs[rp-1].src_pos.first < elems[i].etok)
                  break;
            --rp;  // index of actual rightmost phrase
         }

         // check for target-side contiguity

         pp_span.assign(phrase_pairs.begin()+lp, phrase_pairs.begin()+rp+1);  // copy span
         sortByTarget(pp_span.begin(), pp_span.end());
         if ((elems[i].contig = targetContig(pp_span.begin(), pp_span.end())))
            ++num_elems_contig;

         // check for source-side span match
        
         Uint btok = phrase_pairs[lp].src_pos.first;
         Uint etok = phrase_pairs[rp].src_pos.second;
         Uint btok_tgt = pp_span.begin()->tgt_pos.first;
         Uint etok_tgt = pp_span.back().tgt_pos.second;
         if (verbose)
            cerr << "pp src: " << btok << "," << etok << " tgt: " << btok_tgt << "," << etok_tgt << endl;
         if ((elems[i].palign = btok == elems[i].btok && etok == elems[i].etok))
            ++num_elems_aligned;

         // find element's first target token

         PhrasePair& begpp = *pp_span.begin();  // phrase pair w/ 1st tgt phrase
         if (begpp.src_pos.first == btok) { // 1st tgt phr alig'd to 1st src phr
            if (verbose) cerr << "case 1" << endl;
            alignPhrasePair(dict, anti_dict, begpp, src_toks, tgt_toks, src_al);
            elems[i].btok_tgt = 
               targetSpan(src_al, begpp, elems[i].btok, 
                          min(elems[i].etok, begpp.src_pos.second)).first;
         } else if (begpp.src_pos.second == etok) { // 1st tgt "" end src phr
            if (verbose) cerr << "case 2" << endl;
            alignPhrasePair(dict, anti_dict, begpp, src_toks, tgt_toks, src_al);
            elems[i].btok_tgt = 
               targetSpan(src_al, begpp,
                          begpp.src_pos.first, elems[i].etok).first;
         } else {  // 1st tgt phrase is aligned to middle src phrase
            if (verbose) cerr << "case 3" << endl;
            elems[i].btok_tgt = btok_tgt;
         }

         // find element's last+1 target token

         PhrasePair& endpp = pp_span.back(); // phrase pair w/ last tgt phrase
         if (endpp.src_pos.second == etok) { // end tgt phr alg'd to end src phr
            alignPhrasePair(dict, anti_dict, endpp, src_toks, tgt_toks, src_al);
            elems[i].etok_tgt = 
               targetSpan(src_al, endpp,
                          max(endpp.src_pos.first, elems[i].btok), 
                          elems[i].etok).second;
         } else if (endpp.src_pos.first == btok) { // end tgt phr "" 1st src phr
            alignPhrasePair(dict, anti_dict, endpp, src_toks, tgt_toks, src_al);
            elems[i].etok_tgt = 
               targetSpan(src_al, endpp,
                          elems[i].btok, endpp.src_pos.second).second;
         } else   // end tgt phrase is aligned to middle src phrase
            elems[i].etok_tgt = etok_tgt;

         if (verbose && (elems[i].palign == false || elems[i].contig == false))
            error(ETWarn, "line %d, tag pair %d: %s%s%s", lineno, i+1,
                  !elems[i].palign ? "not aligned with source phrase boundaries" : "", 
                  !elems[i].palign && !elems[i].contig ? "; " : "",
                  !elems[i].contig ? "translation not contiguous" : "");
         if (verbose) elems[i].dump();
      }
      // write out target toks, with elements 

      for (Uint i = 0; i < tgt_toks.size(); ++i) {
         bool tag_out = false;
         for (Uint j = 0; j < elems.size(); ++j) {
            if (elems[j].complete && (xtags || !elems[j].empty()) && elems[j].btok_tgt==i) {
               if (elems[j].lwsl || (!tag_out && elems[j].isIntraTokenLeft()))
                  if (i != 0) cout << ' ';
               cout << elems[j].lstring;
               if (elems[j].btok_tgt == elems[j].etok_tgt && !elems[j].rstring.empty())
                  cout << elems[j].rstring;
               if (elems[j].lwsr)
                  cout << ' ';
               tag_out = true;
            }
         }
         if (!tag_out && i != 0)
            cout << ' ';
         cout << escape(tgt_toks[i]);
         for (Uint j = 0; j < elems.size(); ++j)
            if (elems[j].complete && (xtags || !elems[j].empty()) && elems[j].etok_tgt==i+1)
               if (elems[j].btok_tgt != elems[j].etok_tgt && !elems[j].rstring.empty()) {
                  if (elems[j].rwsl)
                     cout << ' ';
                  cout << elems[j].rstring;
               }
      }
      cout << endl;
   }

   bool outlong = getline(out, line);
   bool pallong = getline(pal, line);
   if (outlong || pallong)
      error(ETFatal, "%s%s%s too long", 
            outlong ? outfile.c_str() : "", 
            outlong && pallong ? " and " : "",
            pallong ? palfile.c_str() : "");

   cerr << lineno << " lines read, "
        << num_unmatched_tags << " unmatched tags, "
        << num_empty_elems << " empty elements, "
        << num_elems << " complete elements: "
        << num_elems_aligned << " aligned with phrase boundaries, "
        << num_elems_contig << " with contiguous translations"
        << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "e", "a", "n:", "xtags"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("e", no_escaping);
   arg_reader.testAndSet("a", align_boundary_phrases);
   arg_reader.testAndSet("n", tags_to_ignore);
   arg_reader.testAndSet("xtags", xtags);

   arg_reader.testAndSet(0, "msrc", msrcfile);
   arg_reader.testAndSet(1, "out", outfile);
   arg_reader.testAndSet(2, "pal", palfile);
}
