/**
 * @author George Foster, updated by Darlene Stewart to reduce memory footprint
 * @file train_tm_mixture.cc
 * @brief  Learn linear weights on the probability columns of a set of input cpts.
 *
 * The input cpts must be sorted with LC_ALL=C.
 *
 * This program should really be split into separate train_tm_mixture and
 * train_dm_mixture parts. Too much code and too many options are specific to
 * these two modes.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, 2012 Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, 2012 Her Majesty in Right of Canada
 */

#include <iostream>
#include <string>
#include <bitset>
#include "exception_dump.h"
#include "printCopyright.h"
#include "arg_reader.h"
#include "phrase_table.h"
#include "em.h"
#include "merge_stream.h"
#include "str_utils.h"
#include "ngram_map.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
train_tm_mixture [options] cpt1 .. cptn jpt\n\
\n\
Learn a set of linear weights over cpt1..cptn that maximizes the probability of\n\
the phrase pairs in jpt according to this mixture. The output is a single cpt\n\
containing the union of the phrase pairs in the input cpts, each of whose\n\
columns is a weighted linear combination of the corresponding columns from the\n\
input cpts (numbers of columns must match across all input cpts). The input\n\
cpts must be sorted with LC_ALL=C, and the output is written in that order too.\n\
\n\
There are two main modes:\n\
\n\
1) If jpt contains a single column of integers, it is assumed to be a joint\n\
phrase table. In this case, the input cpts may contain arbitrary (but\n\
identical) numbers of probability columns, and a set of weights on cpts is\n\
learned separately for each column.\n\
\n\
2) If jpt contains 6 columns of integers, it is assumed to be a distortion\n\
count table. In this case, the input 'cpts' must each contain 6 columns, which\n\
are interpreted as m,s,d orientation probabilities wrt previous and next\n\
phrases respectively. One set of weights on cpts is learned for 'previous'\n\
orientations (first 3 columns), and one for 'next' orientations (second 3\n\
columns). If cpt.bkoff files exist for all input cpts, these are assumed to\n\
contain backoff probabilities, one per column, that are used instead of 0s for\n\
phrase pairs that are absent from the corresponding cpt. A mixture of backoffs\n\
can also be written;  see -o below.\n\
\n\
NB: Smoothing works differently for the two modes. The -s option below is\n\
intended for mode (1), and the bkoff files and -*bkoff and -wb* options are\n\
intended for mode (2). In mode (1) the output mixture will in general not be\n\
normalized, even if -s is given; in mode (2) the output mixture will be\n\
normalized if bkoff files exist and -mixbkoff is given.\n\
\n\
Options:\n\
\n\
-v          Write progress reports to cerr.\n\
-r          Reverse columns of jpt\n\
-s          Pseudo-normalize: in cpts where p(t|s) = 0 AND phrase s is not in\n\
            the cpt, use the avg of p(t|s) across all cpts which do contain s.\n\
            This applies to p(s|t) as well - it assumes that the first n/2\n\
            columns are p(s|t) estimates and the second n/2 are p(t|s). It\n\
            can't be used with dms.\n\
-sc c       Apply a coefficient of c to the -n smoothing avgs. Values in [0,1]\n\
            make sense: 0 implies no -s; 1 gets -s behaviour described above. [1]\n\
-n maxiter  Max number of EM iterations [100]\n\
-prec p     Precision: stop when max change in any weight is < p [0.001]\n\
-w c,wts    Use fixed <wts> for the <c>'th column in the cpt's, rather than\n\
            learning them. Individual weights in <wts> should be separated with\n\
            with commas or blanks. Use -w multiple times for multiple columns.\n\
            In distortion mode (see above), only two 'columns' are allowed, 1\n\
            for 'previous' and 2 for 'next' orientations.\n\
-eq m       Sample all input cpts so they are the same size as the smallest, to\n\
            counter size bias during EM (full cpts are used for final mixture).\n\
            If m = 1, sample by type; if m = 2, simulate sampling by token,\n\
            using jpt frequencies instead of native ones. [0 = no sampling]\n\
-e eps      Use eps for all zero probabilities, if no backoff files exist. [0]\n\
-nobkoff    Ignore cpt.bkoff files if they exist (= backoff to 0 probabilities).\n\
-mixbkoff   Use backoff probabilities when writing the output mixture model. If\n\
            no cpt.bkoff files exist, or if -nobkoff is used, this does nothing.\n\
-wb_in wi   Scale backoff distns by wi (!= 0) when learning (no-op if -nobkoff) [1]\n\
-wb_out wo  Scale backoff distns by wo in output mix (no-op unless -mixbkoff) [1]\n\
-im model   A cpt containing the same phrase pairs as jpt, and the same number of\n\
            columns as the input cpts. During EM, the jpt counts of each phrase\n\
            pair are scaled by the probability from model for the current column.\n\
            NB: this is really intended for distortion mode, in order to spread\n\
            each phrase pair's count mass more smoothly across different\n\
            orientations (it is not well motivated for plain cpts).\n\
-df         During EM, multiply the jpt count of each phrase pair by log(df+1),\n\
            where df is the number of cpts the pair occurs in.\n\
-idf        During EM, multiply the jpt count of each phrase pair by log(T/df),\n\
            where df is the number of cpts the pair occurs in, out of T total.\n\
-write-al A Write alignment information for all phrase pairs having alignments\n\
            in input cpts, after summing frequencies across cpts. If A is 'top',\n\
            write the most frequent alignment without frequency; if 'keep' or\n\
            'all', write all alignments with summed frequencies; if 'none',\n\
            write nothing. [none]\n\
-write-count Write the tallied joint count for each phrase pair in c=<cnt>\n\
            format. [don't]\n\
-o outfile  Write output to outfile. If cpt.bkoff files exist, and -nobkoff is\n\
            not given, then write mixed backoff probabilities to outfile.bkoff.\n\
            [write to stdout, with no backoff probabilities]\n\
-dynout outfile  Instead of a mixed CPT, output the dynamic mixtm specification\n\
            in outfile.mixtm; disables writing the static mix file unless you\n\
            also specify -o. [write only a statically mixed TM]\n\
";

// globals

static const Uint MAX_CPTS_WITH_S = 32;
static bool verbose = false;
static bool rev = false;
static bool smooth_cpts = false;
static double smooth_coeff = 1.0;
static Uint equalize_cpts = 0;  // 1 - no counts, 2+ - use counts
static double eps = 0.0;
static bool nobkoff = false;
static bool mixbkoff = false;
static double wb_in = 1.0;
static double wb_out = 1.0;
static bool df = false;
static bool idf = false;
static Uint maxiter = 100;
static double prec = 0.001;
static vector< vector<double> > fixed_weights;  // col -> wt vect (empty if none)
static vector<string> input_cpt_files;
static string input_jpt_file;
static string indomain_model_file = "";
static string store_alignment_option = "";
static Uint display_alignments = 0; // 0=none, 1=top, 2=all/keep
static bool write_count(false);
static string outfile = "-";
static bool explicit_outfile = false;
static string dynout;
//const string PHRASE_TABLE_SEP = PhraseTableBase::sep + PhraseTableBase::psep +
//                                PhraseTableBase::sep;
// EJJ 2020: I hate to redefine this constant in a hard-coded way, but I'm getting seg faults in
// some circumstances with this code (probably cases where train_tm_mixture.o got initialized before
// phrase_table.o, and we'll never change the phrase table separator, so I'm just going to fix this
// the easy way here:
const string PHRASE_TABLE_SEP = " ||| ";

static void getArgs(int argc, const char* const argv[]);

// Track phrase (not phrase pair) presence in different cpts. Used only if
// smooth_cpts is set.

struct PhrasePresence {
   typedef PhraseTableBase::ToksIter ToksIter;
   NgramMap<Uint> indexes;      // phrase tokens -> index
   vector< bitset<MAX_CPTS_WITH_S> > cpts; // index -> cpt presence

   Uint numPhrases() {return cpts.size();}

   // Record presence of phrase [b,e) in cpt. Return unique index for phrase.
   Uint add(ToksIter b, ToksIter e, Uint cpt) {
      Uint *ind;
      if (!indexes.find_or_insert(b, e, ind)) {
         *ind = cpts.size();
         cpts.push_back(bitset<MAX_CPTS_WITH_S>());
      }
      cpts[*ind].set(cpt);      // set cpt'th bit
      return *ind;
   }

   // Find index for given phrase, or numPhrases() if not there.
   Uint find(ToksIter b, ToksIter e) {
      Uint *pind;
      return indexes.find(b, e, pind) ? *pind : numPhrases();
   }

   // Does a given phrase occur in cpt?
   bool occurs(Uint pind, Uint cpt) {
      assert(pind < cpts.size());
      return cpts[pind].test(cpt);
   }

   // How many cpts does given phrase occur in?
   Uint count(Uint pind) {
      assert(pind < cpts.size());
      return cpts[pind].count();
   }

};

// Globals here because used by Probs and Datum structs below. 
PhrasePresence src_phrases, tgt_phrases;

// Represent phrase-table probabilities for all cpts. We keep the
// probabilities only for the phrase pairs that are found in the JPT.

struct Probs {
   
   vector< vector< vector<float> > > probs; // cpt, phrase, col -> prob
   vector< vector<float> > bkoff_probs; // cpt, col -> prob (0's if bkoff n/a)

   // These are used only if smooth_cpts is set. The src/tgt indexes are from
   // PhrasePresence.
   vector<Uint> src_indexes;    // phrase pair index -> src phrase index
   vector<Uint> tgt_indexes;    // phrase pair index -> tgt phrase index

   Probs(Uint num_cpts, Uint num_phrases = 0) :
      probs(num_cpts), bkoff_probs(num_cpts)
   {
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

   void setSrcTgt(Uint phrase, Uint src, Uint tgt) {
      if (phrase >= src_indexes.size()) src_indexes.resize(phrase+1);
      if (phrase >= tgt_indexes.size()) tgt_indexes.resize(phrase+1);
      src_indexes[phrase] = src;
      tgt_indexes[phrase] = tgt;
   }

   float getProb(Uint cpt, Uint phrase, Uint col) {
      assert(cpt < probs.size() && cpt < bkoff_probs.size());
      if (phrase < probs[cpt].size() && probs[cpt][phrase].size())
         return probs[cpt][phrase][col];
      else
         return bkoff_probs[cpt][col];
   }

   void getPhrases(Uint cpt, vector<Uint>& phrases) {
      phrases.clear();
      for (Uint i = 0; i < probs[cpt].size(); ++i)
         if (probs[cpt][i].size())
            phrases.push_back(i);
   }

   Uint numCpts() {return probs.size();}

   bool phrasePairInSomeCpt(Uint phrase) {
      for (Uint cpt = 0; cpt < probs.size(); ++cpt) {
         assert(phrase < probs[cpt].size());
         if (probs[cpt][phrase].size())
            return true;
      }
      return false;
   }

   //boxing's input start
   double computeDf(Uint phrase) {
      Uint df = 0;
      Uint tot_cpt = probs.size();
      for (Uint cpt = 0; cpt < tot_cpt; ++cpt)
         if (probs[cpt][phrase].size())
            df++;
      double lndf = log((double)(df+0.1));
      return lndf;
   }
   double computeIdf(Uint phrase) {
      Uint df = 0;
      Uint tot_cpt = probs.size();
      for (Uint cpt = 0; cpt < tot_cpt; ++cpt)
         if (probs[cpt][phrase].size())
            df++;
      double lnidf = log((double)tot_cpt/(double)df);
      return lnidf;
   }
   // boxing's input end

   // Pseudo-normalize a vector of probabilities, one per cpt, for a given
   // phrase pair and column. Zero probabilities for cpts that don't contain
   // the conditioning phrase are replaced by the avg over all cpts that do.
   void norm(vector<double>& probs, Uint phrase, Uint col, Uint ncols) {
      Uint pind = col >= ncols/2 ? src_indexes[phrase] : tgt_indexes[phrase];
      PhrasePresence& ppr(col >= ncols/2 ? src_phrases : tgt_phrases);
      double s = 0;
      Uint n = 0;
      for (Uint i = 0; i < probs.size(); ++i)
         if (ppr.occurs(pind, i)) {
            s += probs[i];
            ++n;
         }
      s /= n;
      assert(s);
      s *= smooth_coeff;
      for (Uint i = 0; i < probs.size(); ++i)
         if (!ppr.occurs(pind, i)) {
            assert(probs[i] == 0.0);
            probs[i] = s;
         }
   }
};


// Bundle a phrase and its weighted probabilities for merging cpts using
// mergeStream. Can we call this something other than Datum next time?

struct Datum {
   static Uint num_cpts;
   static vector< vector<double> > cpt_wts; ///< col, cpt -> wt
   static vector< vector<float> > bkoff_probs; ///< col, cpt -> wtd backoff prob
   static bool backoff;         ///< use bkoff_probs for missing entries if true

   string key;                  ///< phrase pair
   string p1, p2;               ///< source and target phrases
   vector<double> probs;        ///< phrase's probabilities
   vector<double> unwtd_probs;  ///< unweighted probs, used if smooth_cpts set 
   bool has_count;              ///< wether a count field was found
   double count;                ///< c= field (phrase pair joint count)
   vector_map<string,Uint> alignments; ///< a= field (phrase's alignments)
   Uint  stream_positional_id;  ///< This Datum is from what positional stream?
   vector<bool> ids_added;      ///< ids from streams +='ed to this one

   vector<char*> toks;         // scratch, but also used by parse if smooth_cpts
   char* destructive_buffer;
   Uint destructive_buffer_size;
   PhraseTableBase::ToksIter b1, b2, e1, e2, v, a, f;


   Datum()
   : probs(1), has_count(false), count(1), stream_positional_id(0)
   , destructive_buffer(NULL), destructive_buffer_size(0) {}

   const string& getKey() const { return key; }

   // Set the values of the static datum parameters: num_cpts, cpt_wts,
   // bkoff_probs, and backoff, respectively. The bprobs parameter is expected
   // to be in the convention used by Probs (cpt, col -> prob), and is
   // converted to (col, cpt -> prob) for consistency with cpt_wts.  The
   // scale_bkoff param is used to scale all backoff probs.
   static void init(Uint ncpts, vector< vector<double> > &wts,
                    vector< vector<float> > &bprobs,
                    bool bkoff, double scale_bkoff)
   {
      num_cpts = ncpts;
      cpt_wts = wts;
      bkoff_probs.resize(cpt_wts.size());
      assert(bprobs.size() == num_cpts);
      for (Uint i = 0; i < bprobs.size(); ++i) {
         assert(bkoff_probs.size() == bprobs[i].size());
         for (Uint j = 0; j < bprobs[i].size(); ++j)
            bkoff_probs[j].push_back(bprobs[i][j]);
      }
      backoff = bkoff;

      // pre-weight and scale the backoff probs
      for (Uint i = 0; i < bkoff_probs.size(); ++i)
         for (Uint j = 0; j < bkoff_probs[i].size(); ++j)
            bkoff_probs[i][j] *= scale_bkoff * cpt_wts[i][j];
   }

   void print(ostream& out) {
      // add pre-weighted backoff probs for missing entries
      if (backoff)
         for (Uint i = 0; i < num_cpts; ++i)
            if (!ids_added[i])
               for (Uint j = 0; j < probs.size(); ++j)
                  probs[j] += bkoff_probs[j][i];
      // replace 0's for missing entries with avgs when conditioning phrase not
      // in table
      if (smooth_cpts) {
         Uint sid = src_phrases.find(b1, e1);
         Uint tid = tgt_phrases.find(b2, e2);
         Uint nsrc_occs = src_phrases.count(sid); // # cpts src phrase found in
         Uint ntgt_occs = tgt_phrases.count(tid); // # cpts tgt phrase found in
         assert (nsrc_occs > 0);
         assert (ntgt_occs > 0);
         Uint n2 = unwtd_probs.size()/2;
         for (Uint i = 0; i < unwtd_probs.size(); ++i) { // average unwtg sums
            unwtd_probs[i] /= (i >= n2 ? nsrc_occs : ntgt_occs);
            unwtd_probs[i] *= smooth_coeff;
         }
         for (Uint i = 0; i < num_cpts; ++i) {
            if (!src_phrases.occurs(sid, i))
               for (Uint j = n2; j < probs.size(); ++j)
                  probs[j] += unwtd_probs[j] * cpt_wts[j][i];
            if (!tgt_phrases.occurs(tid, i))
               for (Uint j = 0; j < n2; ++j)
                  probs[j] += unwtd_probs[j] * cpt_wts[j][i];
         }
      }
      static string a_field;
      a_field = "";
      if (display_alignments && !alignments.empty()) {
         if (display_alignments == 1) {
            a_field = alignments.max()->first;
         } else {
            ostringstream out;
            for (vector_map<string,Uint>::const_iterator it(alignments.begin()), end(alignments.end());
                  it != end;) {
               out << it->first;
               if (it->second != 1 || alignments.size() != 1)
                  out << ":" << it->second;
               if (++it != end) out << ";";
            }
         }
      }
      PhraseTableBase::writePhrasePair(
            out, p1.c_str(), p2.c_str(), // phrase pair
            a_field.empty() ? NULL : a_field.c_str(), // a= field, if any
            probs, // probability values
            write_count && has_count, count); // c= field, if any
   }

   bool parse(const string& buffer, Uint pId) {
      stream_positional_id = pId;
      if (destructive_buffer_size < buffer.size()+1) {
         if (destructive_buffer) delete [] destructive_buffer;
         destructive_buffer_size = buffer.size()+1;
         destructive_buffer = new char[destructive_buffer_size];
      }
      strcpy(destructive_buffer, buffer.c_str());
      PhraseTableBase::extractTokens(buffer, destructive_buffer, toks, b1, e1, b2, e2, v, a, f, true, false);
      p1 = join(b1,e1);
      p2 = join(b2,e2);
      key = p1 + PHRASE_TABLE_SEP + p2 + PHRASE_TABLE_SEP;
      probs.clear();
      unwtd_probs.clear();
      has_count = false;
      count = 1;
      alignments.clear();
      ids_added.assign(num_cpts, 0);
      ids_added[stream_positional_id] = true;
      assert((Uint)(a - v) == cpt_wts.size());
      for (Uint i = 0; v < a; ++v, ++i) {
         probs.push_back(cpt_wts[i][stream_positional_id] * conv<float>(*v));
         if (smooth_cpts) unwtd_probs.push_back(conv<float>(*v));
      }
      for (PhraseTableBase::ToksIter named_field = a; named_field < f; ++named_field) {
         if (display_alignments != 0 && isPrefix("a=", *named_field)) {
            // parse the a= field
            vector<string> all_alignments;
            split((*named_field)+2, all_alignments, ";");
            for ( Uint i = 0; i < all_alignments.size(); ++i ) {
               string::size_type colon_pos = all_alignments[i].find(':');
               if ( colon_pos == string::npos ) {
                  alignments[all_alignments[i]] += 1;
               }
               else {
                  Uint intcount = 0;
                  if ( !convT(all_alignments[i].substr(colon_pos+1).c_str(), intcount) )
                     error(ETWarn, "Count %s is not a number in a= field of %s",
                           all_alignments[i].substr(colon_pos+1).c_str(), buffer.c_str());
                  else
                     alignments[all_alignments[i].substr(0,colon_pos)] += intcount;
               }
            }
         } else if (isPrefix("c=", *named_field)) {
            // parse the c= field
            // note that we do so even if !write_count, because it is used
            // when display_alignments != 0 to weigh each CPT's vote.
            Uint intcount = 0;
            if (!convT((*named_field)+2, intcount)) {
               error(ETWarn, "Count is not a number %s in c= field of %s",
                     (*named_field)+2, buffer.c_str());
            } else {
               has_count = true;
               count = intcount;
            }
         }
      }
      // Typical case: the phrase pair has just one alignment, without count.
      // In that case, set the count to the c= field value.
      if (alignments.size() == 1 && alignments.begin()->second == 1)
         alignments.begin()->second = count;

      return true;
   }

   Datum& operator+=(const Datum& other) {
      assert(key == other.key);
      assert(probs.size() == other.probs.size());
      for (Uint i = 0; i < probs.size(); ++i) 
         probs[i] += other.probs[i];
      if (smooth_cpts)
         for (Uint i = 0; i < unwtd_probs.size(); ++i) 
            unwtd_probs[i] += other.unwtd_probs[i];
      ids_added[other.stream_positional_id] = true;
      if (other.has_count) has_count = true;
      count += other.count;
      alignments += other.alignments;
      return *this;
   }

   bool operator==(const Datum& other) const {
      return key == other.key;
   }

   bool operator<(const Datum& other) const {
      return key < other.key;
   }
};

// Global params for Datum. Blech.
Uint Datum::num_cpts;
vector< vector<double> > Datum::cpt_wts(0);
vector< vector<float> > Datum::bkoff_probs;
bool Datum::backoff = false;


// Stats on individual distns
struct Stats {
   vector<double> lp_totals;    // cpt -> sum of non-zero logprobs
   vector<Uint> num_non_zeros;  // cpt -> number of non-zero logprobs
   Uint num_phrase_pairs;
   
   Stats() {clear();}

   void clear() {
      lp_totals.clear();
      num_non_zeros.clear();
      num_phrase_pairs = 0;
   }
   
   void add(const vector<double>& probs) {
      ++num_phrase_pairs;
      if (lp_totals.size() == 0) {
         lp_totals.assign(probs.size(), 0.0);
         num_non_zeros.assign(probs.size(), 0);
      }
      for (Uint i = 0; i < probs.size(); ++i) {
         if (probs[i]) {
            lp_totals[i] += log(probs[i]);
            ++num_non_zeros[i];
         }
      }
   }

   void report(Uint col) {
      cerr << "column " << col+1 << " ppxs/%non-zero = ";
      for (Uint i = 0; i < lp_totals.size(); ++i) {
         double pc = 100.0 * num_non_zeros[i] / num_phrase_pairs;
         cerr << exp(-lp_totals[i] / num_non_zeros[i]) << "/" << pc << "% ";
      }
      cerr << endl;
   }
};


int MAIN(argc, argv)
{
   printCopyright(2010, "train_tm_mixture");
   getArgs(argc, argv);

   for (vector<string>::iterator p = input_cpt_files.begin();
	p != input_cpt_files.end(); ++p) {
      error_unless_exists(*p, true, "cpt");
   }
   error_unless_exists(input_jpt_file, true, "jpt");

   bool tomodel = false;
   if (indomain_model_file != "") {
      error_unless_exists(indomain_model_file, true, "cpt");
      tomodel = true;
   }

   // Read jpt into pt, using the 'freq' field as an index, and store the joint
   // count in jpt_freqs vector. Optionally read corresponding cpt columns into
   // im_probs.

   PhraseTableGen<Uint> pt;
   Uint num_phrases = 0;

   string line;
   vector<char*> toks;
   PhraseTableBase::ToksIter b1, b2, e1, e2, v, a, f;
   Uint phrase_index;

   vector< vector<Uint> > jpt_freqs;  // index -> vector of joint freqs
   vector< vector<double> > im_probs;  // index -> vector of indomain model probs

   iSafeMagicStream is(input_jpt_file);
   Uint njcols = 0;   // number of cols in jpt: either 1 (jpt) or 6 (dct)
   while (getline(is, line)) {
      char buffer[line.size()+1];
      strcpy(buffer, line.c_str());
      pt.extractTokens(line, buffer, toks, b1, e1, b2, e2, v, a, f, true);
      if (rev) {
         swap(b1, b2);
         swap(e1, e2);
      }
      njcols = static_cast<Uint>(a-v);
      if (! pt.exists(b1, e1, b2, e2, phrase_index)) {
         phrase_index = num_phrases++;
         pt.addPhrasePair(b1, e1, b2, e2, phrase_index);
         jpt_freqs.push_back(vector<Uint>(njcols, 0));
      }
      for (Uint i = 0; i < njcols; ++i)
         jpt_freqs[phrase_index][i] += conv<Uint>(*v++);
   }

   if (tomodel) {
      iSafeMagicStream is(indomain_model_file);
      Uint nimcols = 0;   // number of cols in indomain model
      while (getline(is, line)) {
         char buffer[line.size()+1];
         strcpy(buffer, line.c_str());
         pt.extractTokens(line, buffer, toks, b1, e1, b2, e2, v, a, f, true);
         if (rev) {
            swap(b1, b2);
            swap(e1, e2);
         }
         nimcols = static_cast<Uint>(a-v);
         if (im_probs.empty())
            im_probs.resize(jpt_freqs.size(), vector<double>(nimcols, 0));

         if (! pt.exists(b1, e1, b2, e2, phrase_index)) {
            error(ETFatal, "error - phrase from %s not found in jpt",
                  indomain_model_file.c_str());
         }
         for (Uint i = 0; i < nimcols; ++i)
            im_probs[phrase_index][i] = conv<double>(*v++);
      }
   }

   if (njcols != 1 && njcols != 6)
      error(ETFatal, "jpt contains %d columns - don't know how to handle this!", 
            njcols);
   if (njcols == 6 && smooth_cpts)
      error(ETFatal, "-s option is not compatible with distortion models");
   if (verbose) {
      cerr << "read " << input_jpt_file << " - " << jpt_freqs.size()
           << " pairs" << endl;
      cerr << "number of jpt columns: " << njcols;
      cerr << " - mixing " << (njcols == 1 ? "conditional phrase tables" : "distortion models");
      cerr << endl;
   }

   // Read contents of all cpts, keeping probabilities in Probs struct only for
   // phrases found in pt. Optionally keep track of which cpts each src and tgt
   // phrase occur in.

   Probs probs(input_cpt_files.size(), num_phrases);
   Uint num_cols = 0;

   // all_weights_fixed may be incorrectly true until the number of columns is known.
   bool all_weights_fixed = true;
   for (Uint i = 0; i < fixed_weights.size() && all_weights_fixed; ++i)
      if (fixed_weights[i].empty()) all_weights_fixed = false;
   Uint num_bkoff_files = 0;
   vector<Uint> orig_cpt_sizes(input_cpt_files.size(), 0);
   vector<Uint> filt_cpt_sizes(input_cpt_files.size(), 0);

   for (Uint i = 0; i < input_cpt_files.size(); ++i) {
      iSafeMagicStream istr(input_cpt_files[i]);

      while (getline(istr, line)) {
         ++orig_cpt_sizes[i];
         char buffer[line.size()+1];
         strcpy(buffer, line.c_str());
         pt.extractTokens(line, buffer, toks, b1, e1, b2, e2, v, a, f, true, false);
         if (num_cols != 0) {
            if (static_cast<Uint>(a - v) != num_cols)
               error(ETFatal, "phrasetables must have same numbers of columns!");
         } else {
            num_cols = a - v;
            if (fixed_weights.size() < num_cols)
               all_weights_fixed = false;
         }
         Uint src_ind = smooth_cpts ? src_phrases.add(b1, e1, i) : 0;
         Uint tgt_ind = smooth_cpts ? tgt_phrases.add(b2, e2, i) : 0;
         if (all_weights_fixed && !smooth_cpts) break; // don't need to read the cpt
         if (pt.exists(b1, e1, b2, e2, phrase_index)) {
            ++filt_cpt_sizes[i];
            for (Uint j = 0; j < num_cols; ++j)
               probs.setProb(i, phrase_index, j, num_cols, conv<float>(*v++));
            if (smooth_cpts) probs.setSrcTgt(phrase_index, src_ind, tgt_ind);
         }
      }
      if (verbose)
         cerr << "read " << input_cpt_files[i] << " - probabilities for "
              << filt_cpt_sizes[i] << "/" << orig_cpt_sizes[i] << " pairs retained" <<  endl;

      string bkoff_file(removeZipExtension(input_cpt_files[i])+".bkoff");
      if (!nobkoff && check_if_exists(bkoff_file)) {
         string buf;
         split(gulpFile(bkoff_file.c_str(), buf), probs.bkoff_probs[i]);
         if (probs.bkoff_probs[i].size() != num_cols)
            error(ETFatal, "number of backoff probs in %s doesn't match number of columns in %s",
                  bkoff_file.c_str(), input_cpt_files[i].c_str());
         for (Uint j = 0; j < probs.bkoff_probs[i].size(); ++j)
            probs.bkoff_probs[i][j] *= wb_in;  // scale backoff distribution
         if (verbose)
            cerr << "   using backoff probabilities from " << bkoff_file << endl;
         ++num_bkoff_files;
      } else                    // backoff to eps probs
         probs.bkoff_probs[i].assign(num_cols, eps);
   }

   if (num_bkoff_files && num_bkoff_files != input_cpt_files.size())
      error(ETWarn, "backoff files found for only %d/%d input tables",
            num_bkoff_files, input_cpt_files.size());
   if (njcols == 6 && num_cols != 6)
      error(ETFatal, "%s has 6 columns, so it's a dct - %s",
            input_jpt_file.c_str(), "hence all dms must also have 6 columns");

   // 'equalize' cpt sizes

   if (equalize_cpts) {
      Uint min_size = *min_element(orig_cpt_sizes.begin(), orig_cpt_sizes.end());
      vector<Uint> phrase_indexes, counts, sum_counts;
      for (Uint i = 0; i < orig_cpt_sizes.size(); ++i) {
         probs.getPhrases(i, phrase_indexes);
         counts.resize(phrase_indexes.size());
         sum_counts.resize(phrase_indexes.size());
         Uint size = 0;
         for (Uint j = 0; j < phrase_indexes.size(); ++j) {
            vector<Uint>& freqs = jpt_freqs[phrase_indexes[j]];
            counts[j] = equalize_cpts == 1 ? 1 :
               accumulate(freqs.begin(), freqs.end(), 0);
            sum_counts[j] = counts[j];
            if (j) sum_counts[j] += sum_counts[j-1];
            size += counts[j];
         }
         double proportion = min_size / double(orig_cpt_sizes[i]);
         Uint num_to_remove = size - proportion * size;
         if (verbose) 
            cerr << "sampling " << input_cpt_files[i] << " for EM: retaining " 
                 << proportion * 100 << "% of " << size << " matched phrases" 
                 << endl;
         for (Uint j = 0; j < num_to_remove; ++j) {
            Uint p = rand() % size;
            vector<Uint>::iterator it = upper_bound(sum_counts.begin(), sum_counts.end(), p);
            assert(it != sum_counts.end());
            Uint k = it - sum_counts.begin();
            if (counts[k] == 0) // try again
               ++num_to_remove;
            else
               --counts[k];
         }
         for (Uint j = 0; j < phrase_indexes.size(); ++j) {
            if (counts[j] == 0) {
               assert(probs.probs[i][phrase_indexes[j]].size());
               probs.probs[i][phrase_indexes[j]].clear();
            }
         }
      }
   }

   Uint tot_jpt_freq = 0;
   for (Uint i = 0; i < num_phrases; ++i) {
      if (njcols == 1) {
         if (probs.phrasePairInSomeCpt(i)) tot_jpt_freq += jpt_freqs[i][0];
      } else  // sum of 1st three dct cols == sum of 2nd three dct cols
         for (Uint j = 0; j < 3; ++j)
            tot_jpt_freq += jpt_freqs[i][j];
   }

   // EM

   if (njcols == 6)
      num_cols = 2;  // one 'column' for prev probs, one for next probs
   if (fixed_weights.size() > num_cols)
      error(ETFatal, "-w column spec too large: %d; must be <= number of columns: %d",
            fixed_weights.size(), num_cols);
   vector< vector<double> > wts(num_cols);   // col, cpt -> wt
   for (Uint i = 0; i < num_cols; ++i) {
      vector<double> pr(probs.numCpts());  // prob vector for current column
      if (verbose) {
         if (njcols == 1)
            cerr << "EM for column " << i+1 << ": " << endl;
         else
            cerr << "EM for " << (i == 0 ? "'prev'" : "'next'") << " probs: " << endl;
      }
      if (i < fixed_weights.size() && fixed_weights[i].size()) {
         if (verbose) cerr << "(skipping - using fixed weights)" << endl;
         wts[i] = fixed_weights[i];
      } else {
         Stats stats;
         EM em(probs.numCpts());
         for (Uint iter = 0; iter < maxiter; ++iter) {
            double lp = 0.0, tot_smoothedcount = 0.0;
            for (Uint j = 0; j < jpt_freqs.size(); ++j) {
               if (!probs.phrasePairInSomeCpt(j)) continue;
               if (njcols == 1) {  // cpt mixture
                  for (Uint k = 0; k < pr.size(); ++k)
                     pr[k] = probs.getProb(k, j, i);
                  if (smooth_cpts) probs.norm(pr, j, i, num_cols);
                  double smoothedcount = jpt_freqs[j][0];
                  if (tomodel) smoothedcount *= im_probs[j][i];
                  if (df) smoothedcount *= probs.computeDf(j);
                  if (idf) smoothedcount *= probs.computeIdf(j);
                  lp += smoothedcount * log(em.count(pr, smoothedcount));
                  tot_smoothedcount += smoothedcount;
                  if (verbose && iter == 0) stats.add(pr);
               } else {            // dm mixture
                  for (Uint r = 3*i; r < 3*(i+1); ++r) {  // column index in 0..5
                     for (Uint k = 0; k < pr.size(); ++k)
                        pr[k] = probs.getProb(k, j, r);
                     double smoothedcount = jpt_freqs[j][r];
                     if (tomodel) {
                        Uint tot_freq = 0;
                        for (Uint m = 3*i; m < 3*(i+1); ++m)
                           tot_freq += jpt_freqs[j][m];
                        smoothedcount = tot_freq * im_probs[j][r];
                     }
                     if (df) smoothedcount *= probs.computeDf(j);
                     if (idf) smoothedcount *= probs.computeIdf(j);
                     lp += smoothedcount * log(em.count(pr, smoothedcount));
                     tot_smoothedcount += smoothedcount;
                  }
               }
            }
            if (verbose) {
               if (iter == 0 && njcols == 1) {
                  stats.report(i);
                  stats.clear();
               }
               cerr << "iter " << iter+1 << " done: ppx = "
                    << exp(-lp / tot_smoothedcount) << endl;
            }
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

   // If combining dms, we have one weight for the three 'prev' orientations,
   // and one for the 3 'next' orientations. These each need to be replicated
   // three times, to give 6 column-wise weights to keep the table combination
   // code happy.
   if (njcols == 6) {
      vector< vector<double> > dupwts(6);   // col, cpt -> wt
      for (Uint i = 0; i < 6; ++i)
         dupwts[i] = wts[i < 3 ? 0 : 1];
      wts = dupwts;
      num_cols = 6;
   }

   if (!dynout.empty()) {
      // In -dynout mode, we write the model parameters out as a dynamic .mixtm model
      oSafeMagicStream dynoutFile(dynout == "-" ? "-" : dynout+".mixtm");
      dynoutFile << "Portage dynamic MixTM v1.0" << endl;
      for (Uint k = 0; k < input_cpt_files.size(); ++k) {
         dynoutFile << input_cpt_files[k] << "\t";
         for (Uint i = 0; i < num_cols; ++i) {
            if (i > 0) dynoutFile << " ";
            assert(wts[i].size() == input_cpt_files.size());
            dynoutFile << wts[i][k];
         }
         dynoutFile << endl;
      }
   }

   if (dynout.empty() || explicit_outfile) {
      // Read the cpts again, merging them using the weights computed above, and
      // write the output cpt.

      Datum::init(input_cpt_files.size(), wts, probs.bkoff_probs, mixbkoff, wb_out / wb_in);
      mergeStream<Datum> ms(input_cpt_files);
      oSafeMagicStream ostr(outfile);
      Uint num_pairs_written = 0;
      while (!ms.eof()) {
         ms.next().print(ostr);
         ++num_pairs_written;
      }

      // Optionally write weighted backoff probs

      if (!nobkoff && num_bkoff_files) {
         vector<double> mix_bkoff_probs(wts.size(), 0.0);
         for (Uint i = 0; i < wts.size(); ++i)
            for (Uint j = 0; j < wts[i].size(); ++j)
               mix_bkoff_probs[i] += wts[i][j] * probs.bkoff_probs[j][i] / wb_in;
         if (outfile != "-") {
            string bkoff_file(removeZipExtension(outfile)+".bkoff");
            oSafeMagicStream os(bkoff_file);
            copy(mix_bkoff_probs.begin(), mix_bkoff_probs.end(),
                 ostream_iterator<float>(os, " "));
            os << endl;
        }
         if (verbose) {
            cerr << "weighted backoff probs: ";
            copy(mix_bkoff_probs.begin(), mix_bkoff_probs.end(),
                 ostream_iterator<float>(cerr, " "));
            cerr << endl;
         }
      }

      if (verbose)
         cerr << "wrote " << num_pairs_written << " phrase pairs" << endl;
   }
}
END_MAIN


void getArgs(int argc, const char* const argv[])
{
   const char* switches[] = {"v", "r", "s", "sc:", "df", "idf", "eq:", 
                             "e:", "nobkoff", "mixbkoff", "write-count", "write-al:",
                             "wb_in:", "wb_out:", "prec:", "n:", "w:", "o:", "im:", "dynout:"};

   vector<string> fixed_weights_strs;

   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 2, -1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("r", rev);
   arg_reader.testAndSet("s", smooth_cpts);
   arg_reader.testAndSet("sc", smooth_coeff);
   arg_reader.testAndSet("df", df);
   arg_reader.testAndSet("idf", idf);
   arg_reader.testAndSet("eq", equalize_cpts);
   arg_reader.testAndSet("e", eps);
   arg_reader.testAndSet("nobkoff", nobkoff);
   arg_reader.testAndSet("mixbkoff", mixbkoff);
   arg_reader.testAndSet("wb_in", wb_in);
   arg_reader.testAndSet("wb_out", wb_out);
   arg_reader.testAndSet("n", maxiter);
   arg_reader.testAndSet("prec", prec);
   arg_reader.testAndSet("w", fixed_weights_strs);
   explicit_outfile = arg_reader.getSwitch("o", &outfile);
   arg_reader.testAndSet("write-al", store_alignment_option);
   arg_reader.testAndSet("write-count", write_count);
   arg_reader.testAndSet("im", indomain_model_file);
   arg_reader.testAndSet("dynout", dynout);

   arg_reader.getVars(0, input_cpt_files);
   input_jpt_file = input_cpt_files.back();
   input_cpt_files.pop_back();

   if (wb_in == 0.0)
      error(ETFatal, "Please use a non-zero value for -wb_in");

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

   if (smooth_cpts && input_cpt_files.size() > MAX_CPTS_WITH_S)
      error(ETFatal, "At most %d cpts can be used with -s", MAX_CPTS_WITH_S);

   if (!store_alignment_option.empty()) {
      if ( store_alignment_option == "top" ) {
         // display_alignments==1 means display only top one, without freq
         display_alignments = 1;
      } else if ( store_alignment_option == "keep" || store_alignment_option == "all" ) {
         // display_alignments==2 means display all alignments with freq.  In
         // joint2cond_phrase_tables, this means preserving whatever was in jtable.
         display_alignments = 2;
      } else if ( store_alignment_option == "none" ) {
         display_alignments = 0;
      } else {
         error(ETFatal, "Invalid -write-al value: %s; expected top or keep.",
               store_alignment_option.c_str());
      }
   }

   /*
   if (!dynout.empty()) {
      if (arg_reader.getSwitch("o"))
         error(ETWarn, "Ignoring -o switch when using -dynout.");
      if (!store_alignment_option.empty())
         error(ETWarn, "Ignoring -write-al switch when using -dynout.");
      if (write_count)
         error(ETWarn, "Ignoring -write-count switch when using -dynout.");
   }
   */
}
