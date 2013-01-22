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
#include "str_utils.h"
#include "word_align_io.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
markup_canoe_output [options] MSRC OUT PAL\n\
\n\
Add XML-style tags in source file MSRC to canoe output OUT, using phrase-\n\
and word-alignment file PAL to determine proper placement.\n\
\n\
MSRC is a version of the source file SRC fed to canoe, in which optional tags\n\
have been added, in the form:\n\
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
   canoe -f canoe.ini -palign [-walign] < SRC |\n\
      nbest2rescore.pl -canoe -palout=PAL [-wal] > OUT\n\
\n\
With -xtags, empty tags are transferred too, and paired tags (e.g. bpt, ept)\n\
are swapped when necessary to preserve correct tag ordering.\n\
\n\
Options:\n\
  -v    Write progress reports to cerr.\n\
  -vv   Be extra verbose.\n\
  -a    Do word alignment to determine target span even when phrase boundaries\n\
        line up with tag boundaries [word-align only when boundaries don't align]\n\
  -wal WAL    Indicate how to obtain word-alignment information.  One of:\n\
        h     Use heuristic word alignment only.\n\
        pal   Use word alignments from the PAL file only (ultimately from the\n\
              a= field in phrase tables); with pal, it is an error if the PAL\n\
              file is missing the word alignment of a multi-word phrase.\n\
        mixed Use word alingments from the PAL file when found, heuristics\n\
              otherwise.\n\
        [mixed]\n\
  -n T  Treat all tags of the form <T...> or </T> as ordinary tokens.  May be\n\
        repeated to specify multiple tags. Warning: the tokenization implied by\n\
        this option must match that described in the PAL file - beware of\n\
        problems caused by tags that consist of multiple elements or ones that\n\
        abut normal tokens.\n\
  -e    Don't escape XML reserved characters <& in output. [Escape these chars\n\
        except when in added tags or those specified by -n.]\n\
  -xtags Handle formatting XML tags from TMX and SDLXLIFF file formats [don't]\n\
";

// globals

static bool verbose = false;
static bool extra_verbose = false;
static bool align_boundary_phrases = false;
static bool no_escaping = false;
static bool xtags = false;
static string msrcfile;
static string outfile;
static string palfile;
static string wal = "mixed";
static bool hwal = false;
static bool pal_wal = false;
static vector<string> tags_to_ignore;
static void getArgs(int argc, char* argv[]);

// represent an XML element: pair of tags, with token positions, possibly
// identical (if element is empty)

struct Elem {

   string name;                 // element name
   string lstring;              // left tag string, eg '<name atr="xxx">'
   string rstring;              // right tag string, eg '</name>'
   Uint lid;                    // id of left tag (used for tag ordering)
   Uint rid;                    // id of right tag (used for tag ordering)
   Uint btok;                   // index of 1st src tok to the right of left tag
   Uint etok;                   // index of 1st src tok to the right of right tag
   bool complete;               // true if right tag seen (& etok is valid)
   bool lwsl;                   // true if whitespace to left of left tag
   bool lwsr;                   // true if whitespace to right of left tag
   bool lwslseq;                // true if whitespace to left of sequence of left tags
   bool lwsrseq;                // true if whitespace to right of sequence of left tags
   bool rwsl;                   // true if whitespace to left of right tag
   bool rwsr;                   // true if whitespace to right of right tag
   bool rwslseq;                // true if whitespace to left of sequence of right tags
   bool rwsrseq;                // true if whitespace to right of sequence of right tags

   bool lout;                   // true if left tag has been output already

   bool palign;                 // elem is aligned w/ src-side phrase boundaries
   bool contig;                 // elem's tgt phrases are contiguous
   Uint btok_tgt;               // index of 1st tgt tok to the right of left tag
   Uint etok_tgt;               // index of 1st tgt tok to the right of right tag
   string pair_id;              // pair-id if elem is paired with another (e.g. bx/ex)

   static Uint next_id;         // Next tag id to assign

   // create, from left tag positions within line
   Elem(const string& line, Uint beg, Uint end, Uint btok, bool wslseq, bool wsrseq, Uint nest) :
      name(getName(line, beg, end)),
      lstring(line.substr(beg, end-beg)), rstring(""), lid(next_id++), rid(0),
      btok(btok), etok(0), complete(false),
      lwsl(beg==0 || line[beg-1]==' '), lwsr(end==line.size() || line[end]==' '),
      lwslseq(wslseq), lwsrseq(wsrseq),
      rwsl(true), rwsr(true), rwslseq(false), rwsrseq(false),
      lout(false), btok_tgt(0), etok_tgt(0), pair_id("")
   {
      if (xtags)
         if (name == "open_wrap" || name == "close_wrap" ||
             name == "bx" || name == "ex") {
            XMLishTag tag;
            parseXMLishTag(lstring.c_str(), tag, NULL, NULL);
            pair_id = tag.attrVal("id");
         }
   }

   static bool isLeft(const string& line, Uint beg, Uint end) {
      return line[beg+1] != '/';
   }
   
   static bool isRight(const string& line, Uint beg, Uint end) {
      return line[beg+1] == '/' || line[end-2] == '/';
   }

   static string getName(const string& line, Uint beg, Uint end) {
      const size_t s = line[beg+1] == '/' ? beg+2 : beg+1;
      const size_t e = line.find_first_of(" />\t\n", s);
      assert (e != string::npos && e < end);
      return line.substr(s, e-s);
   }
   
   void dump() {
      cerr << name << " " << "(" << lid << "," << rid << "): "
           << lstring << " " << rstring
           << (complete ? " " : "in") << "complete,"
           << (pair_id.empty() ? "" : " pair id: ") << pair_id
           << (isOpenPair() ? " (open)" : "")
           << (isClosePair() ? " (close)" : "")
           << " src:(" << btok << "," << etok << ")"
           << " tgt:(" << btok_tgt << "," << etok_tgt << ")"
           << " whitespace: " << lwsl << lwsr << rwsl << rwsr
           << " seq ws: " << lwslseq << lwsrseq << rwslseq << rwsrseq
           << endl;
   }

   bool empty() { return btok == etok; }

   bool emptyTgt() { return btok_tgt == etok_tgt; }

   // Does this tag element meet the requirements for transfer to target text?
   bool shouldTransfer() { return complete && (xtags || !empty()); }

   bool isIntraTokenLeft() { return !lwslseq && !lwsrseq; }

   bool isIntraTokenRight() { return !rwslseq && !rwsrseq; }

   bool isIntraToken() { return isIntraTokenLeft() || isIntraTokenRight(); }

   bool isPaired() { return !pair_id.empty(); }

   // open_wrap or bx ?
   bool isOpenPair() { return isPaired() && (name[0] == 'o' || name[0] == 'b'); }

   // close_wrap of ex ?
   bool isClosePair() { return isPaired() && (name[0] == 'c' || name[0] == 'e'); }

   // is tag identified by id nested within this element in the source?
   bool nested(Uint id) { return id > lid && id < rid; }

   // update, from right tag positions within line
   void update(const string& line, Uint beg, Uint end, Uint etok_tag,
               bool wslseq, bool wsrseq)
   {
      if (line[beg+1] == '/') {
         rstring = line.substr(beg, end-beg);
         rid = next_id++;
      }
      etok = etok_tag;
      complete = true;
      rwsl = beg==0 || line[beg-1]==' ';
      rwsr = end==line.size() || line[end]==' ';
      rwslseq = wslseq;
      rwsrseq = wsrseq;
   }
};

Uint Elem::next_id = 1;         // tag ids start at 1

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
 * If hwal is not given on the command line and the PAL file had alignment
 * info, return the actual alignment; otherwise heuristically align words
 * within an aligned phrase pair. Very simple 1-way alignment.
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
                     vector<vector<Uint> >& src_al) 
{
   if (!pp.alignment.empty() && !hwal) {

      GreenReader('_')(pp.alignment, src_al);

   } else {

      static char punc_num[] = "0123456789=[];,./!@#$%^&*()_+{}:<>?";
      
      Uint slen = pp.src_pos.second - pp.src_pos.first;
      Uint tlen = pp.tgt_pos.second - pp.tgt_pos.first;

      if (pal_wal && (slen != 1 || tlen != 1))
         error(ETFatal, "Missing word-alignment information in the PAL file.  Make sure your phrase table has the a= field, that canoe has -walign, and that nbest2rescore.pl has -wal.");

      src_al.resize(slen, vector<Uint>(1,0));
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
            if(extra_verbose) {
                cerr << "alignPhrasePair: i=" << i
                     << " slen=" << slen
                     << " first=" << pp.src_pos.first << " second=" << pp.src_pos.second
                     << " src_toks.size()=" << src_toks.size()
                     << " j=" << j
                     << " tlen=" << tlen
                     << " first=" << pp.tgt_pos.first << " second=" << pp.tgt_pos.second
                     << " tgt_toks.size()=" << tgt_toks.size()
                     << endl;
                cerr << "alignPhrasePair: stok: " << stok << " ttok: " << ttok
                     << " score: " << score << " src_scores[" << i << "]=" << src_scores[i]
                     << endl;
            }
            if (score > src_scores[i]) {
               if(extra_verbose)
                   cerr << "alignPhrasePair: setting src_all[" << i << "]=" << j << endl;
               src_scores[i] = score;
               src_al[i][0] = j;
             }
         }
         if(extra_verbose)
             cerr << "alignPhrasePair: src_all[" << i << "]=" << join(src_al[i]) << endl;
      }
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
pair<Uint,Uint> targetSpan(const vector<vector<Uint> >& src_al, const PhrasePair& pp,
                           Uint beg, Uint end)
{
   if (!align_boundary_phrases && 
       beg == pp.src_pos.first && end == pp.src_pos.second)
      return make_pair(pp.tgt_pos.first, pp.tgt_pos.second);

   // point tag
   if (beg == end) {
      Uint min_j = pp.tgt_pos.second-pp.tgt_pos.first;
      for (Uint i = beg-pp.src_pos.first; i < src_al.size(); ++i)
         for (Uint k = 0; k < src_al[i].size(); ++k)
            min_j = min(min_j, src_al[i][k]);
      return make_pair(min_j+pp.tgt_pos.first, min_j+pp.tgt_pos.first);
   }

   assert(pp.tgt_pos.second > pp.tgt_pos.first);
   pair<Uint,Uint> ret(make_pair(pp.tgt_pos.second-pp.tgt_pos.first, 0));
   for (Uint i = beg-pp.src_pos.first; i < end-pp.src_pos.first; ++i) {
      for (Uint j = 0; j < src_al[i].size(); ++j) {
         if (src_al[i][j] < ret.first) ret.first = src_al[i][j];
         if (src_al[i][j] >= ret.second) ret.second = src_al[i][j] + 1;
      }
   }

   // Gracefully (though not always correctly) handle the case where no word
   // in the source span has a link.
   if (ret.first > ret.second) {
      if (beg == pp.src_pos.first)
         ret.first = ret.second;
      else
         ret.second = ret.first;
   }

   ret.first += pp.tgt_pos.first;
   ret.second += pp.tgt_pos.first;

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
   vector<Uint> nested;     // stack of nested elems during output
   vector<string> src_toks;
   vector<string> tgt_toks;
   vector<vector<Uint> > src_al;
   vector<PhrasePair> phrase_pairs;
   vector<PhrasePair> pp_span;
   string line;
   Uint lineno = 0;
   Uint num_unmatched_tags = 0;
   Uint num_bad_nesting = 0;
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
      nested.clear();
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
            wsr = p2 < line.size() ? (whitespace.find(line[p2]) != string::npos) : true;
         }
         string name = Elem::getName(line, beg, end);
         if (ignoreTag(name)) {
            splitAndMerge(line.substr(p, end-p), src_toks, is_intra_tok_tag);
            p = end;
            continue;
         }
         splitAndMerge(line.substr(p, beg-p), src_toks, is_intra_tok_tag);
         if (Elem::isLeft(line, beg, end)) {
            is_intra_tok_tag = !wsl && !wsr;
            Uint btok = src_toks.size() - (is_intra_tok_tag ? 1 : 0);
            Uint nest = nested.empty() ? 0 : nested.back()+1;
            nested.push_back(elems.size());
            elems.push_back(Elem(line, beg, end, btok, wsl, wsr, nest));
         }
         if (Elem::isRight(line, beg, end)) { // find a match
            Uint i = elems.size();
            for (; i > 0; --i) {
               if (!elems[i-1].complete && elems[i-1].name == name) {
                  is_intra_tok_tag = !wsl && !wsr;
                  Uint etok = src_toks.size() - (is_intra_tok_tag ? 1 : 0);
                  elems[i-1].update(line, beg, end, etok, wsl, wsr);
                  break;
               }
            }
            if (i == 0) {
               string tag = line.substr(beg, end-beg);
               if (verbose)
                  error(ETWarn, "line %d: %s has no matching left tag - ignoring",
                        lineno, tag.c_str());
               ++num_unmatched_tags;
            } else {
               // i-1 is the index of the element we are working with
               if (!nested.empty() && nested.back() == i-1) {
                  nested.pop_back();
               } else {
                  for (Uint j=nested.size(); j > 0; --j)
                     if (nested[j-1] == i-1) {
                        nested.erase(nested.begin()+j-1);
                        break;
                     }
                  if (verbose)
                     error(ETWarn, "line %d: XML tag nesting bad within element %i: %s",
                           lineno, i, elems[i-1].lstring.c_str());
                  ++num_bad_nesting;
               }
               if (elems[i-1].isClosePair() ) {  // find match for paired element
                  Uint j = i-1;
                  for (; j > 0; --j) {
                     if (elems[j-1].pair_id == elems[i-1].pair_id) {
                        // set src token span for both paired elements.
                        // after the tgt span is determined, these will be restored.
                        elems[j-1].etok = elems[i-1].etok;
                        elems[i-1].btok = elems[j-1].btok;
                        break;
                     }
                  }
                  if (j == 0) {
                     if (verbose)
                        error(ETWarn, "line %d: Paired element %s has no matching opening element",
                              lineno, elems[i-1].lstring.c_str());
                     ++num_unmatched_tags;
                  }
               }
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
         error(ETFatal, "line %d: %s specifies wrong number of source tokens.  Expected %d tokens, got %d tokens: %s",
               lineno, palfile.c_str(),
               phrase_pairs.size() ? phrase_pairs.back().src_pos.second : 0,
               src_toks.size(),
               join(src_toks, " | ").c_str());

      if (extra_verbose) {
         cerr << "src tokens: " << join(src_toks, " | ") << endl;
         cerr << "tgt tokens: " << join(tgt_toks, " | ") << endl;
      }

      // assign target positions to elements
      for (Uint i = 0; i < elems.size(); ++i) {
         if (extra_verbose) elems[i].dump();
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

         if (lp < phrase_pairs.size()) {
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
               if (verbose) cerr << "begin case 1" << endl;
               alignPhrasePair(dict, anti_dict, begpp, src_toks, tgt_toks, src_al);
               elems[i].btok_tgt =
                  targetSpan(src_al, begpp, elems[i].btok,
                             min(elems[i].etok, begpp.src_pos.second)).first;
            } else if (begpp.src_pos.second == etok) { // 1st tgt "" end src phr
               if (verbose) cerr << "begin case 2" << endl;
               alignPhrasePair(dict, anti_dict, begpp, src_toks, tgt_toks, src_al);
               elems[i].btok_tgt =
                  targetSpan(src_al, begpp,
                             begpp.src_pos.first, elems[i].etok).first;
            } else {  // 1st tgt phrase is aligned to middle src phrase
               if (verbose) cerr << "begin case 3" << endl;
               elems[i].btok_tgt = btok_tgt;
            }

            // find element's last+1 target token
            PhrasePair& endpp = pp_span.back(); // phrase pair w/ last tgt phrase
            if (endpp.src_pos.second == etok) { // end tgt phr alg'd to end src phr
               if (verbose) cerr << "end case 1" << endl;
               alignPhrasePair(dict, anti_dict, endpp, src_toks, tgt_toks, src_al);
               elems[i].etok_tgt =
                  targetSpan(src_al, endpp,
                             max(endpp.src_pos.first, elems[i].btok),
                             elems[i].etok).second;
            } else if (endpp.src_pos.first == btok) { // end tgt phr "" 1st src phr
               if (verbose) cerr << "end case 2" << endl;
               alignPhrasePair(dict, anti_dict, endpp, src_toks, tgt_toks, src_al);
               elems[i].etok_tgt =
                  targetSpan(src_al, endpp,
                             elems[i].btok, endpp.src_pos.second).second;
            } else {  // end tgt phrase is aligned to middle src phrase
               if (verbose) cerr << "end case 3" << endl;
               elems[i].etok_tgt = etok_tgt;
            }

         } else {
            // There can be (point) tags can be at after last token.
            elems[i].contig = true;
            ++num_elems_contig;
            elems[i].palign = true;
            ++num_elems_aligned;
            elems[i].btok_tgt = elems[i].etok_tgt = tgt_toks.size();
         }

         if (verbose && (elems[i].palign == false || elems[i].contig == false))
            error(ETWarn, "line %d, tag pair %d: %s%s%s", lineno, i+1,
                  !elems[i].palign ? "not aligned with source phrase boundaries" : "", 
                  !elems[i].palign && !elems[i].contig ? "; " : "",
                  !elems[i].contig ? "translation not contiguous" : "");
         if (extra_verbose) elems[i].dump();
      } // for each element

      // Now that the target spans are set, restore the btok, etok and
      // btok_tgt, etok_tgt fields for paired elements such that that pair
      // open tag will be output before the pair close tag.
      if (xtags) {
         for (Uint i = 0; i < elems.size(); ++i) {
            if (elems[i].isOpenPair()) {
               elems[i].etok = elems[i].btok;
               elems[i].etok_tgt = elems[i].btok_tgt;
            } else if (elems[i].isClosePair()) {
               elems[i].btok = elems[i].etok;
               elems[i].btok_tgt = elems[i].etok_tgt;
            }
         }
      }

      // write out target tokens, with elements
      nested.clear();
      bool need_ws = false;
      bool seq_start = true;
      Uint nested_mark = 0;     // used to mark a spot in the nested stack
      bool rwsr = false;        // rwsr for last right tag output
      bool sp_out = false;      // was last output character a space?
      for (Uint i = 0; i < tgt_toks.size(); ++i) {
         // Output left tags for elements beginning at this token;
         // include right tags for empty elements before this token.
         // Obey the nesting of empty tags.
         seq_start = true;
         nested_mark = nested.size();   // mark this spot in the nested stack
         rwsr = false;    // rwsr for last right tag output
         sp_out = false;  // was last output character a space?
         for (Uint j = 0; j < elems.size(); ++j) {
            if (elems[j].shouldTransfer() && elems[j].btok_tgt==i) {
               if (!elems[j].isIntraTokenLeft() && !elems[j].lout) {
                  // Output right tags if needed for empty tags in sequence,
                  // including moving intra-token right tag before this token.
                  while (nested.size() > nested_mark) {
                     Uint k = nested.back();
                     if (elems[k].nested(elems[j].lid))
                        break;
                     if (elems[k].rwsl && !sp_out) {
                        cout << ' ';
                        need_ws = false;
                     }
                     cout << elems[k].rstring;
                     sp_out = false;
                     if (i != 0 || elems[k].btok == 0)
                        rwsr = elems[k].rwsr || elems[k].rwsrseq;
                     nested.pop_back();
                  }
                  if (need_ws && (elems[j].lwsl || (seq_start && elems[j].lwslseq))) {
                     cout << ' ';
                     need_ws = false;
                  }
                  // Output this left tag.
                  cout << elems[j].lstring;
                  sp_out = false;
                  elems[j].lout = true;
                  if (!elems[j].rstring.empty()) {
                     // Push element on nested stack to output right tag later.
                     nested.push_back(j);
                     if (!elems[j].emptyTgt())
                        nested_mark = nested.size();   // mark this spot in the nested stack
                     // Note: usually, don't want whitespace before the first token.
                     if (elems[j].lwsr && (i != 0 || elems[j].btok == 0)) {
                        cout << ' ';
                        sp_out = true;
                        need_ws = false;
                     }
                  } else {
                     // Point tag - track whitespace needs to right.
                     if (i != 0 || elems[j].btok == 0)
                        rwsr = elems[j].rwsr || elems[j].rwsrseq;
                  }
                  seq_start = false;
               }
            }
         }

         // Finish outputting right tags for empty tags before this token,
         // including moving intra-token right tag before this token.
         while (nested.size() > nested_mark) {
            Uint k = nested.back();
            if (elems[k].rwsl && !sp_out) {
               cout << ' ';
               need_ws = false;
            }
            cout << elems[k].rstring;
            sp_out = false;
            if (i != 0 || elems[k].btok == 0)
               rwsr = elems[k].rwsr || elems[k].rwsrseq;
            nested.pop_back();
         }
         if (rwsr) {
            cout << ' ';
            need_ws = false;
         }

         // Output the token, preceded by whitespace if needed.
         if (need_ws)
            cout << ' ';
         cout << escape(tgt_toks[i]);
         need_ws = true;
         // Output (move) intra-token left tags after this token.
         for (Uint j = 0; j < elems.size(); ++j) {
            if (elems[j].shouldTransfer() && elems[j].btok_tgt==i) {
               if (elems[j].isIntraTokenLeft() && !elems[j].lout) {
                  cout << elems[j].lstring;
                  elems[j].lout = true;
                  if (!elems[j].rstring.empty())
                     nested.push_back(j);
               }
            }
         }

         // Output right tags for elements ending at this token.
         // Note: Proper XML nesting order is maintained in the output, but
         // some right tags may be output (moved to) later than indicated by
         // the alignment in order to remove overlap between elements.
         rwsr = false;     // rwsr for last right tag output
         nested_mark = nested.size();   // mark this spot in the nested stack
         while (!nested.empty()) {
            Uint j = nested.back();
            if (nested.size() <= nested_mark &&
                  elems[j].etok_tgt > i + (elems[j].empty() ? 0 : 1))
               break;
            if (nested.size() == nested_mark)
               --nested_mark;
            // Output empty (point) elements that must precede this right tag
            for (Uint k=j; k < elems.size(); ++k) {
               if (elems[k].shouldTransfer() && elems[k].empty() && !elems[k].lout) {
                  if (((!elems[k].isPaired() && elems[k].btok == elems[j].etok) ||
                       (elems[k].isClosePair() && elems[k].btok_tgt == i+1))
                      && elems[j].nested(elems[k].lid)) {
                     if (need_ws && elems[k].lwsl) {
                        cout << ' ';
                        need_ws = false;
                     }
                     cout << elems[k].lstring;
                     elems[k].lout = true;
                     if (!elems[k].rstring.empty()) {
                        nested.push_back(k);
                        j = k;
                     }
                  }
               }
            }
            // Output this right tag.
            if (elems[j].rwsl) {
               cout << ' ';
               need_ws = false;
            }
            cout << elems[j].rstring;
            rwsr = elems[j].rwsr || elems[j].rwsrseq;
            nested.pop_back();
         }
         // Output tags for any closing paired elements ending at this token
         // not output by the right tag processing above.
         // Paired elements are handled outside the nesting structure
         // i.e. paired elements are not pushed on the nested stack.
         for (Uint j = 0; j < elems.size(); ++j) {
            if (elems[j].isClosePair() && !elems[j].lout) {
               if (elems[j].shouldTransfer() && elems[j].btok_tgt==i+1) {
                  if (elems[j].lwsl) {
                     cout << ' ';
                     need_ws = false;
                  }
                  cout << elems[j].lstring;
                  elems[j].lout = true;
                  rwsr = elems[j].rwsr || elems[j].rwsrseq;
               }
            }
         }
         // Output whitespace if needed after right/pair-closing tags.
         if (rwsr && i < tgt_toks.size()-1) {
            cout << ' ';
            need_ws = false;
         }
      } // for each token

      // Finally, output any tags that follow the last token.
      seq_start = true;
      rwsr = false;    // rwsr for last right tag output
      sp_out = false;  // was last output character a space?
      for (Uint j = 0; j < elems.size(); ++j) {
         // Output right tags if needed for empty tags in sequence.
         while (!nested.empty()) {
            Uint k = nested.back();
            if (elems[k].nested(elems[j].lid))
               break;
            if (elems[k].rwsl && !sp_out)
               cout << ' ';
            cout << elems[k].rstring;
            sp_out = false;
            nested.pop_back();
         }
         if (!elems[j].lout && elems[j].shouldTransfer() && elems[j].btok_tgt==tgt_toks.size()) {
            if (need_ws && (elems[j].lwsl || (seq_start && elems[j].lwslseq)))
               cout << ' ';
            cout << elems[j].lstring;
            elems[j].lout = true;
            if (!elems[j].rstring.empty()) {
               nested.push_back(j);
               if (elems[j].lwsr) {
                  cout << ' ';
                  sp_out = true;
               }
            }
            seq_start = false;
         }
      }
      // Finish outputting right tags for empty tags.
      while (!nested.empty()) {
         Uint k = nested.back();
         if (elems[k].rwsl && !sp_out)
            cout << ' ';
         cout << elems[k].rstring;
         nested.pop_back();
      }

      // Finally, we are at the end of the line!
      cout << endl;

   } // for each line of input

   bool outlong = getline(out, line);
   bool pallong = getline(pal, line);
   if (outlong || pallong)
      error(ETFatal, "%s%s%s too long", 
            outlong ? outfile.c_str() : "", 
            outlong && pallong ? " and " : "",
            pallong ? palfile.c_str() : "");

   cerr << lineno << " lines read, "
        << num_unmatched_tags << " unmatched tags, "
        << num_bad_nesting << " improperly nested elements, "
        << num_empty_elems << " empty elements, "
        << num_elems << " complete elements: "
        << num_elems_aligned << " aligned with phrase boundaries, "
        << num_elems_contig << " with contiguous translations"
        << endl;
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "vv", "e", "a", "n:", "xtags", "wal:"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 3, 4, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("vv", extra_verbose);
   if (extra_verbose) verbose = true;
   arg_reader.testAndSet("e", no_escaping);
   arg_reader.testAndSet("a", align_boundary_phrases);
   arg_reader.testAndSet("n", tags_to_ignore);
   arg_reader.testAndSet("xtags", xtags);
   arg_reader.testAndSet("wal", wal);

   if (wal == "h") hwal = true;
   else if (wal == "pal") pal_wal = true;
   else if (wal != "mixed")
      error(ETFatal, "Unrecognized value for -wal: '%s'; valid values are h, pal, and mixed", wal.c_str());

   arg_reader.testAndSet(0, "msrc", msrcfile);
   arg_reader.testAndSet(1, "out", outfile);
   arg_reader.testAndSet(2, "pal", palfile);
}
