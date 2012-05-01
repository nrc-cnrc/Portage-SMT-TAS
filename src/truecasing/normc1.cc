/**
 * @author George Foster with extensions by Darlene Stewart
 * @file normc1.cc 
 * @brief Case-normalize the first character of lines in a text file.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, 2011 Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, 2011 Her Majesty in Right of Canada
 */
#include <iostream>
#include <fstream>
#include <lm.h>
#include <logging.h>
#include <casemap_strings.h>
#include "arg_reader.h"
#include "printCopyright.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
normc1 [-v][-loc loc] lmfile infile [outfile]\n\
\n\
Perform case normalization of sentence-initial characters, eg:\n\
\n\
   A flat in London .        -> a flat in London .\n\
   London is on the Thames . -> London is on the Thames .\n\
\n\
<lm> must be a language model trained using train-nc1.sh. <infile> should be in\n\
original case and tokenized, with possibly more than one sentence per line.\n\
\n\
Warning:\n\
  You cannot use stdin or - or a pipe as the infile!\n\
Options:\n\
\n\
-v   Write progress reports to cerr.\n\
-loc Use locale value loc to perform case conversion. Ex: fr_CA.iso88591,\n\
     C, or en_US.UTF-8. [value of LC_CTYPE from environment]\n\
-ignore IG Ignore tokens at sentence start (BOS) according to IG. [0]\n\
     0 - skip only: ) \" \n\
     1 - skip all non cased tokens\n\
-extended Use extended EOS punctuation ('-',':'). [no]\n\
-notitle Don't normalize title lines. [normalize titles]\n\
";

// globals

static bool verbose = false;
static string lmfile;
static string loc("");
static string infile = "-";
static string outfile = "-";
static int ignore = 0;
static bool extended_eos_punc = false;
static bool norm_titles = true;


static void getArgs(int argc, char* argv[]);

// local wrapper around case-handling routines

static inline bool isCapitalized(const CaseMapStrings& cms, const string& s, const string& l)
{
   return s == cms.capitalize(l);
}

static inline bool isCapitalized(const CaseMapStrings& cms, const string& s)
{
   return isCapitalized(cms, s, cms.toLower(s));
}

static inline bool isCased(const string& l, const string& u)
{
   return l != u;
}

static inline bool isCased(const CaseMapStrings& cms, const string& s)
{
   return isCased(cms.toLower(s), cms.toUpper(s));
}

bool isNumber(const string &s)
{
   // very crude number matching - lots of false positives
   return s.find_first_not_of("0123456789.,-") == string::npos;
}

bool isTime(const string &s)
{
   // very crude time matching - lots of false positives
   return s.find_first_not_of("0123456789:") == string::npos;
}

bool isEosPunc(const vector<string> &tokens, const Uint idx, const bool extended_punc)
{
   const string& tok = tokens[idx];
   if (tok == "." || tok == "!" || tok == "?")
      return true;
   if (not extended_punc)
      return false;
   if (tok == ":")
      return true;
   if (tok == "-" || tok == "–" || tok == "—") {
      if (idx > 0 and idx+1 < tokens.size())
         if ((isNumber(tokens[idx-1]) and isNumber(tokens[idx+1])) or isTime(tokens[idx+1]))
            // don't treat 'number - number' as EOS.
            // don't treat '- time' as EOS (time-time difficult to match).
            return false;
      return true;
   }
   return false;
}

bool isTitle(const CaseMapStrings& cms, const vector<string> &tokens, const Uint idx)
{
   Uint cnt_cased = 1;
   Uint cnt_all = 1;
   Uint cnt_contiguous = 1;
   Uint cnt_cont_or_short = 1;
   Uint cnt_short_seq = 0;
   Uint cnt_short_non_cap = 0;
   bool fnd_non_cap = false;
   bool fnd_long_non_cap = false;
   string l, u;
   for (Uint i = idx+1; i < tokens.size(); ++i) {
      const string &tok = tokens[i];
      if (isEosPunc(tokens, i, false))
      {
         return false;
      }
      if (isEosPunc(tokens, i, extended_eos_punc))
         break;
      l = cms.toLower(tok);
      u = cms.toUpper(tok);
      if (isCased(l, u)) {
         ++cnt_cased;
         if (isCapitalized(cms, tok, l) or tok == u) { // treat all uppercase as capitalized
            ++cnt_all;
            if (not fnd_non_cap) ++cnt_contiguous;
            if (not fnd_long_non_cap) {
               cnt_cont_or_short += cnt_short_seq + 1;
               cnt_short_seq = 0;
            }
         } else {
            fnd_non_cap = true;
            if (tok.size() >= 5)
               fnd_long_non_cap = true;
            else {
               ++cnt_short_non_cap;
               // **** Fix to not count short after last Cap!!!!!
               if (not fnd_long_non_cap) ++cnt_short_seq;
            }
         }
       }
   }

   int ret;
   if (cnt_contiguous < 2)
      ret = 0;
   else if (cnt_contiguous > 4 or (cnt_cont_or_short == cnt_cased) or cnt_cont_or_short > 6)
      ret = 1;
   else
      ret = 0;
   return ret > 0;
}

int main(int argc, char* argv[])
{
   printCopyright(2007, "normc1");

   Logging::init();
   getArgs(argc, argv);

   CaseMapStrings cms(loc.c_str());
   if (verbose)
      cerr << "using locale: " << cms.localeName() << endl;

   // Add all input text tokens to the vocabulary.
   VocabFilter vf(0);
   if (true)
   {
      iSafeMagicStream istr(infile);
      string line;
      vector<string> tokens;
      while (getline(istr, line)) {
         tokens.clear();
         split(line.c_str(), tokens);
         for (Uint i=0; i<tokens.size(); ++i) {
            // The input has some uppercased words, we also want the lowercase
            // version of those tokens in our vocabulary.
            vf.add(tokens[i].c_str());
            vf.add(cms.toLower(tokens[i]).c_str());
         }
      }
   }

   iSafeMagicStream istr(infile);
   oSafeMagicStream ostr(outfile);

   PLM* lm = PLM::Create(lmfile, &vf, PLM::SimpleAutoVoc, -INFINITY,
                         true, 0, NULL); 
   const Uint n = lm->getOrder();

   string line, norm, upper;
   vector<string> tokens;
   Uint context[n];

   while (getline(istr, line)) {
      splitZ(line, tokens);
      bool start = true;
      for (Uint i = 0; i < tokens.size(); ++i) {
         const string& f = tokens[i];
         cms.toLower(f, norm);
         cms.toUpper(f, upper);
         if (start && isCased(norm, upper) && isCapitalized(cms, f, norm) &&
             (norm_titles or not isTitle(cms, tokens, i))) {
            // try to normalize
            Uint j;
            // we don't reverse context here, because lm is itself reversed:
            for (j = 0; j < n && i+j+1 < tokens.size(); ++j)
               context[j] = vf.index(tokens[i+j+1].c_str());
            cms.toLower(f, norm);
            const float cap_prob = lm->wordProb(vf.index(f.c_str()), context, j);
            const float low_prob = lm->wordProb(vf.index(norm.c_str()), context, j);
            ostr << (low_prob > cap_prob ? norm : f);
         } else
            ostr << f;

         if (i+1 != tokens.size()) ostr << ' ';

         start = isEosPunc(tokens, i, extended_eos_punc) ||
                 (start && (ignore ? !isCased(norm, upper) : f == ")" || f == "\""));
      }
      ostr << endl;
   }
}

// arg processing

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "loc:", "ignore:", "extended", "notitle"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 1, 3, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("loc", loc);
   arg_reader.testAndSet("ignore", ignore);
   arg_reader.testAndSet("extended", extended_eos_punc);
   bool no_norm_titles = false;
   arg_reader.testAndSet("notitle", no_norm_titles);
   norm_titles = !no_norm_titles;

   arg_reader.testAndSet(0, "lmfile", lmfile);
   arg_reader.testAndSet(1, "infile", infile);
   arg_reader.testAndSet(2, "outfile", outfile);

   if (infile == "-" or infile == "/dev/stdin")
      error(ETFatal, "The input file cannot be - or /dev/stdin since we need to read the input file sevaral times.");
}
