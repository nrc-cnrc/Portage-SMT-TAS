/**
 * @author George Foster, updated by Darlene Stewart to reduce memory footprint
 * @file train_tm_mixture.cc
 * @brief  Learn linear weights on the probability columns of a set of input cpts.
 *
 * The input cpts must be sorted with LC_ALL=C.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, 2012 Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, 2012 Her Majesty in Right of Canada
 */

#include <iostream>
#include <string>
#include "exception_dump.h"
#include "printCopyright.h"
#include "arg_reader.h"
#include "phrase_table.h"
#include "em.h"
#include "merge_stream.h"
#include "str_utils.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
train_tm_mixture [options] cpt1 .. cptn jpt\n\
\n\
Learn a set of linear weights over cpt1..cptn that maximizes the probability of\n\
the phrase pairs in jpt according to this mixture. Each cpt is assumed to\n\
contain m probability columns, and a separate weight vector is learned for each\n\
column. Phrase pairs that are absent from some cpts are assigned 0 probability\n\
in those cpts. The input cpts must be sorted with LC_ALL=C. The output is a\n\
single cpt containing the union of the phrase pairs in the input cpts along\n\
with m probability columns, each of which is a weighted linear combination of\n\
the corresponding columns from the input cpts. The output cpt is in LC_ALL=C\n\
sort order.\n\
\n\
Options:\n\
\n\
-v          Write progress reports to cerr.\n\
-r          Reverse columns of jpt\n\
-n maxiter  Max number of EM iterations [100]\n\
-prec p     Precision: stop when max change in any weight is < p [0.001]\n\
-w c,wts    Use fixed <wts> for the <c>'th column in the cpt's, rather than\n\
            learning them. Individual weights in <wts> should be separated with\n\
            with commas or blanks. Use -w multiple times for multiple columns.\n\
-o outfile  Write output to outfile [write to stdout]\n\
";

// globals
static bool verbose = false;
static bool rev = false;
static Uint maxiter = 100;
static double prec = 0.001;
static vector< vector<double> > fixed_weights;  // col -> wt vect (empty if none)
static vector<string> input_cpt_files;
static string input_jpt_file;
static string outfile = "-";
const string PHRASE_TABLE_SEP = PhraseTableBase::sep + PhraseTableBase::psep +
                                PhraseTableBase::sep;

static void getArgs(int argc, const char* const argv[]);

// Represent phrase-table probabilities
// We keep the probabilities only for the phrase pairs that are found in the JPT.
struct Probs {
   // cpt, phrase, col -> prob
   vector< vector< vector<float> > > probs;

   Probs(Uint num_cpts, Uint num_phrases = 0) : probs(num_cpts) {
      if (num_phrases > 0)
         for (Uint cpt = 0; cpt < num_cpts; ++cpt)
            probs[cpt].resize(num_phrases);
   }

   void setProb(Uint cpt, Uint phrase, Uint col, Uint ncols, float prob) {
      assert(cpt < probs.size());
      if (phrase >= probs[cpt].size())  // no-op when sized in constructor
         probs[cpt].resize(phrase+1);
      if (col >= probs[cpt][phrase].size())
         probs[cpt][phrase].resize(ncols);
      probs[cpt][phrase][col] = prob;
   }

   float getProb(Uint cpt, Uint phrase, Uint col) {
      assert(cpt < probs.size());
      if (phrase < probs[cpt].size() && probs[cpt][phrase].size())
         return probs[cpt][phrase][col];
      else
         return 0.0;
   }

   Uint numCpts() {return probs.size();}

   bool phraseInCpt(Uint phrase) {
      for (Uint cpt = 0; cpt < probs.size(); ++cpt)
         if (probs[cpt][phrase].size())
            return true;
      return false;
   }
};


// Bundle a phrase and its weighted probabilities for merging cpts using mergeStream.
struct Datum {
   static vector< vector<double> > cpt_wts; ///< cpt weights: col, cpt -> wt
   string key;                  ///< phrase
   string p1, p2;               ///< source and target phrases
   vector<double> probs;        ///< phrase's probabilities
   Uint  stream_positional_id;  ///< This Datum is from what positional stream?

   Datum()
   : probs(1), stream_positional_id(0)
   {}

   const string& getKey() const { return key; }

   static void initWeights(vector< vector<double> > &wts) {
      cpt_wts = wts;
   }

   void print(ostream& out) {
      PhraseTableBase::writePhrasePair(out, p1.c_str(), p2.c_str(), NULL, probs, false, 0.0);
   }

   bool parse(const string& buffer, Uint pId) {
      stream_positional_id = pId;
      vector<string> toks;
      PhraseTableBase::ToksIter b1, b2, e1, e2, v, a, f;

      PhraseTableBase::extractTokens(buffer, toks, b1, e1, b2, e2, v, a, f, true, false);

      // Verify on the fly that the stream is LC_ALL=C.
      p1 = join(b1,e1);
      p2 = join(b2,e2);
      key = p1 + PHRASE_TABLE_SEP + p2 + PHRASE_TABLE_SEP;

      probs.clear();
      assert((Uint)(a - v) == cpt_wts.size());
      for (Uint i = 0; v < a; ++v, ++i) {
         probs.push_back(cpt_wts[i][stream_positional_id] * conv<float>(*v));
      }

      return true;
   }

   Datum& operator+=(const Datum& other) {
      assert(key == other.key);
      assert(probs.size() == other.probs.size());
      for (Uint i = 0; i < probs.size(); ++i) {
         probs[i] += other.probs[i];
      }
      return *this;
   }

   bool operator==(const Datum& other) const {
      return key == other.key;
   }

   bool operator<(const Datum& other) const {
      return key < other.key;
   }
};

vector< vector<double> > Datum::cpt_wts(0);


int MAIN(argc, argv)
{
   printCopyright(2010, "train_tm_mixture");
   getArgs(argc, argv);

   for (vector<string>::iterator p = input_cpt_files.begin();
	p != input_cpt_files.end(); ++p) {
      error_unless_exists(*p, true, "cpt");
   }

   error_unless_exists(input_jpt_file, true, "jpt");

   // read in jpt into pt, using the 'freq' field as an index,
   // and store the joint count in jpt_freqs vector.
   PhraseTableGen<Uint> pt;
   Uint num_phrases = 0;

   string line;
   vector<string> toks;
   PhraseTableBase::ToksIter b1, b2, e1, e2, v, a, f;
   Uint phrase_index;

   vector<Uint> jpt_freqs;     // corresponding joint freqs

   iSafeMagicStream is(input_jpt_file);
   while (getline(is, line)) {
      pt.extractTokens(line, toks, b1, e1, b2, e2, v, a, f);
      if (rev) {
         swap(b1, b2);
         swap(e1, e2);
      }
      if (! pt.exists(b1, e1, b2, e2, phrase_index)) {
         phrase_index = num_phrases++;
         pt.addPhrasePair(b1, e1, b2, e2, phrase_index);
         jpt_freqs.push_back(0);
      }
      jpt_freqs[phrase_index] += conv<Uint>(*v);
   }
   if (verbose)
      cerr << "read " << input_jpt_file << " - " << jpt_freqs.size()
           << " pairs" << endl;

   // read contents of all cpts, keeping probabilities in Probs struct
   // only for phrases found in pt
   Probs probs(input_cpt_files.size(), num_phrases);
   Uint num_cols = 0;

   // all_weights_fixed may be incorrectly true until the number of columns is known.
   bool all_weights_fixed = true;
   for (Uint i = 0; i < fixed_weights.size() && all_weights_fixed; ++i)
      if (fixed_weights[i].empty()) all_weights_fixed = false;

   for (Uint i = 0; i < input_cpt_files.size(); ++i) {
      iSafeMagicStream istr(input_cpt_files[i]);
      Uint prob_cnt = 0;

      // read at least one line from each cpt to check the number of columns.
      while (getline(istr, line)) {
         pt.extractTokens(line, toks, b1, e1, b2, e2, v, a, f, true, false);
         if (num_cols != 0) {
            if (static_cast<Uint>(a - v) != num_cols)
               error(ETFatal, "phrasetables must have same numbers of columns!");
         } else {
            num_cols = a - v;
            if (fixed_weights.size() < num_cols)
               all_weights_fixed = false;
         }
         if (all_weights_fixed) break; // don't need to read the cpt
         if (pt.exists(b1, e1, b2, e2, phrase_index)) {
            ++prob_cnt;
            for (Uint j = 0; j < num_cols; ++j)
               probs.setProb(i, phrase_index, j, num_cols, conv<float>(*v++));
         }
      }
      if (verbose)
         cerr << "read " << input_cpt_files[i] << " - probabilities for "
              << prob_cnt << " pairs retained" <<  endl;
   }

   Uint tot_jpt_freq = 0;
   for (Uint i = 0; i < num_phrases; ++i)
      if (probs.phraseInCpt(i)) tot_jpt_freq += jpt_freqs[i];

   if (fixed_weights.size() > num_cols)
      error(ETFatal, "-w column spec too large: %d; must be <= number of columns: %d",
            fixed_weights.size(), num_cols);

   // em
   vector< vector<double> > wts(num_cols);   // col, cpt -> wt
   for (Uint i = 0; i < num_cols; ++i) {
      vector<double> pr(probs.numCpts());  // prob vector for current column
      if (verbose)
          cerr << "EM for column " << i+1 << ": " << endl;
      if (i < fixed_weights.size() && fixed_weights[i].size()) {
         if (verbose) cerr << "(skipping - using fixed weights)" << endl;
         wts[i] = fixed_weights[i];
      } else {
         EM em(probs.numCpts());
         for (Uint iter = 0; iter < maxiter; ++iter) {
            double lp = 0.0;
            for (Uint j = 0; j < jpt_freqs.size(); ++j) {
               if (probs.phraseInCpt(j)) {
                  for (Uint k = 0; k < pr.size(); ++k)
                     pr[k] = probs.getProb(k, j, i);
                  lp += jpt_freqs[j] * em.count(pr, jpt_freqs[j]);
               }
            }
            if (verbose)
               cerr << "iter " << iter+1 << " done: ppx = "
                    << exp(-lp / tot_jpt_freq) << endl;
            if (em.estimate() < prec)
               break;
         }
         wts[i] = em.getWeights();
      }
      if (verbose) {
         cerr << "column " << i+1 << " wts = ";
         for (Uint k = 0; k < wts[i].size(); ++k)
            cerr << wts[i][k] << ' ';
         cerr << endl;
      }
   }


   // Read the cpts again, merging them using the weights computed above, and
   // write the output cpt.
   Datum::initWeights(wts);
   mergeStream<Datum> ms(input_cpt_files);
   oSafeMagicStream ostr(outfile);
   // EJJ High precision is fine and all, but costs us in space in CPT files,
   // as well as in TPPT files, where the encoding is more expensive to keep
   // all those pointless bits around.
   //ostr.precision(9);
   Uint num_pairs_written = 0;
   while (!ms.eof()) {
      ms.next().print(ostr);
      ++num_pairs_written;
   }

   if (verbose)
      cerr << "wrote " << num_pairs_written << " phrase pairs" << endl;
}
END_MAIN


void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "r", "prec:", "n:", "w:", "o:"};

   vector<string> fixed_weights_strs;

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", rev);
   arg_reader.testAndSet("n", maxiter);
   arg_reader.testAndSet("prec", prec);
   arg_reader.testAndSet("w", fixed_weights_strs);
   arg_reader.testAndSet("o", outfile);

   arg_reader.getVars(0, input_cpt_files);
   input_jpt_file = input_cpt_files.back();
   input_cpt_files.pop_back();

   vector<string> toks;
   for (Uint i = 0; i < fixed_weights_strs.size(); ++i) {
      splitZ(fixed_weights_strs[i], toks, " ,", 2);
      if (toks.size() != 2)
         error(ETFatal, "-w argument (%s) is not in the right format: c,wts (comma/space separated)",
               fixed_weights_strs[i].c_str());
      Uint c = conv<Uint>(toks[0]);
      if (c == 0)
         error(ETFatal, "-w column number must be > 0");
      c = c - 1;
      while (fixed_weights.size() <= c)
         fixed_weights.push_back(vector<double>(0));
      splitCheckZ(toks[1], fixed_weights[c], " ,");
      if (fixed_weights[c].size() != input_cpt_files.size())
         error(ETFatal, "number of weights in -w argument (%s) must match number of cpts: %d",
               fixed_weights_strs[i].c_str(), input_cpt_files.size());
   }
}
