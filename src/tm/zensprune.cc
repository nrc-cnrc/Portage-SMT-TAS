/**
 * @author George Foster
 * @file zensprune.cc
 * @brief Based on Zens et al, EMNLP 2012.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <iostream>
#include <fstream>
#include <bitset>
#include <list>
#include <algorithm>
#include <omp.h>
#include "file_utils.h"
#include "arg_reader.h"
#include "printCopyright.h"
#include "phrase_table.h"

using namespace Portage;
using namespace std;

static char help_message[] = "\n\
zensprune [options] [cpt]\n\
\n\
Prune a conditional phrase table based on compositionality scores, and write\n\
results to stdout. The cpt must include joint counts in the 3rd column c= field\n\
(in order to allow for exceptions, this is not a hard requirement, but pruning\n\
won't work as intended without counts). The output table preserves input phrase\n\
order, provided source phrases are grouped initially.\n\
\n\
Options:\n\
\n\
-v     Write progress reports to cerr.\n\
-o     Write pruning scores after each phrase (after all other fields).\n\
-r     Assume that cpt is in the format written by -o, and prune based on the\n\
       scores in the final field.\n\
-j     Use joint counts rather than compositionality scores for pruning.\n\
-e     Use expected joint counts rather than compositionality scores for\n\
       pruning. This uses forward and backward conditional probabilities\n\
       from columns specified by -ec. It is not compatible with -j.\n\
-ec c  Use the cth pair of probability columns for -e. For example, if there\n\
       are 6 columns, 1 designates pair 1,4; 2 designates 2,5; etc. [2]\n\
-l     Save memory by reading the input table twice. Recommended for -r and -j;\n\
       not compatible with -m or when the input is not a file. If source\n\
       phrases are not grouped in the input, this option writes output in a\n\
       different order from the default (it preserves input order exactly).\n\
-s     Use colum s for logprobs. This is a 1-based index into the list of all\n\
       probabilities and adirectional scores in the order they appear. If an\n\
       adirectional score is used, it must have a probabilistic interpretation.\n\
       [0 = use 1st 'forward' prob]\n\
-pX n  How much of the table to retain. X must be one of: r - keep top n%;\n\
       a - keep top n pairs, t - keep pairs with score >= n [-pr 50]\n\
-c     Viterbi logprob to use for non-compositional phrase pairs [-10]\n\
-m n   Filter out pairs with counts < n. This is done while reading the\n\
       table, so the -pX settings apply to the resulting set of pairs. [0]\n\
-i l   Use ITG decomposition for phrase pairs whose shortest phrase contains\n\
       l or more words. This changes the output. [0 = no ITG]\n\
";

// globals

static const Uint MAX_PHRASE_LEN = 16;
static Uint max_hyps_used = 0;  // during Viterbi search

static bool verbose = false;
static bool debug = false;
static bool write_scores = false;
static bool read_scores = false;
static bool count_pruning = false;
static bool exp_count_pruning = false;
static Uint exp_count_col = 2;
static bool low_mem = false;
static string infile("-");
static Uint score_col = 0;
static double prune_val = 50.0;
static enum {Abs, Rel, Thresh} prune_type = Rel;
static double default_vit_logpr = -10;
static double min_count = 0;
static Uint itg_thresh = 0;
static bool itg_fast = false;

// Info stored against each phrase pair
struct PhraseInfo {
   float logpr;        // log prob of key value
   float vit_logpr;    // log prob of viterbi decomp (complete score if -r or -e)
   Uint count;         // joint count
   const char* vals;   // values associated with this pair (null if low_mem)
   float score() {
      return count_pruning ? count :
         (read_scores || exp_count_pruning) ? vit_logpr : 
            count * (logpr - vit_logpr);
   }
   PhraseInfo(float logpr = 0, Uint count=1) :
      logpr(logpr), vit_logpr(default_vit_logpr), count(count), vals(NULL) {}
};

// Compare functor for sorting PhraseInfo indexes.
struct ComparePhraseInfos {
   vector<PhraseInfo>& infos;
   bool operator()(const Uint& a, const Uint& b)
      {return infos[a].score() > infos[b].score();}
   ComparePhraseInfos(vector<PhraseInfo>& infos) : infos(infos) {}
};

static void getArgs(int argc, char* argv[]);
static bool computeViterbiScore(PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                                PhraseTableGen<Uint>::iterator p, PhraseInfo& pi);

static void getScore(string& line, float* score);
static Uint getJointCount(PhraseTableBase::ToksIter a, PhraseTableBase::ToksIter f,
                          const string& line);

// main

int main(int argc, char* argv[])
{
   printCopyright(2012, "zensprune");
   getArgs(argc, argv);
   bool computing_scores = 
      !count_pruning && !exp_count_pruning && !read_scores; // convenience
   if (verbose) 
      cerr << (low_mem ? "" : "not ") << "using low-memory algorithm" << endl;

   iSafeMagicStream istr(infile);
   PhraseTableGen<Uint> pt;  // src,tgt -> index
   vector<PhraseInfo> infos;    // index -> prob & string info
   vector<Uint> src_counts;     // src-phrase index -> count
   vector<Uint> tgt_counts;     // tgt-phrase index -> count

   string line, vals;
   vector<char*> toks;
   PhraseTableBase::ToksIter b1, b2, e1, e2, v, a, f;

   // read original table to extract scoring info

   if (verbose) cerr << "reading " << infile << endl;
   Uint num_filtered = 0;
   while (getline(istr, line)) {
      PhraseInfo pi;
      if (read_scores) getScore(line, &pi.vit_logpr);
      char buffer[line.size()+1];
      strcpy(buffer, line.c_str());
      pt.extractTokens(line, buffer, toks, b1, e1, b2, e2, v, a, f, true, true);
      pi.count = getJointCount(a, f, line);
      if (pi.count < min_count) {
         ++num_filtered;
         continue;
      }
      if (computing_scores) { // need to read logpr
         Uint nv = static_cast<Uint>(a-v);
         if (score_col == 0) score_col = nv / 2 + 1;  // 1st fwd prob
         if (score_col - 1 < nv)
            pi.logpr = log(conv<float>(*(v+score_col-1)));
         else if (f != toks.end() && (score_col - 1 - nv < static_cast<Uint>(toks.end()-f-1)))
            pi.logpr = log(conv<float>(*(f+score_col-nv)));
         else 
            error(ETFatal, "no value column %d for phrase pair: %s", score_col,
               line.c_str());
         if (static_cast<Uint>(e2-b2) > MAX_PHRASE_LEN)
            error(ETFatal, "phrase contains > %d target tokens: %s", 
               MAX_PHRASE_LEN, line.c_str());
      } else if (exp_count_pruning) { // read bkw,fwd cond probs & store in pi
         Uint nv = static_cast<Uint>(a-v);
         if (exp_count_col > 2 * nv)
            error(ETFatal, "no probability column pair %d for phrase pair: %s",
                  exp_count_col, line.c_str());
         pi.logpr = conv<float>(v[exp_count_col-1]); // p(src|tgt)
         pi.vit_logpr = conv<float>(v[exp_count_col-1 + nv/2]); // p(tgt|src)
      }
      if (computing_scores || exp_count_pruning || !low_mem) {  // need src,tgt -> index
         pair<Uint,Uint> st = pt.addPhrasePair(b1, e1, b2, e2, infos.size());
         if (exp_count_pruning) {   // increment marginals
            if (st.first >= src_counts.size()) src_counts.push_back(0);
            if (st.second >= tgt_counts.size()) tgt_counts.push_back(0);
            assert(st.first < src_counts.size());
            assert(st.second < tgt_counts.size());
            src_counts[st.first] += pi.count;
            tgt_counts[st.second] += pi.count;
         }
      }
      if (!low_mem) {  // need to store values string
         vals = join(v, PhraseTableBase::ToksIter(toks.end()));
         pi.vals = strdup_new(vals.c_str());
         assert(pi.vals);
      }
      infos.push_back(pi);
   }
   if (verbose) cerr << infos.size() << " phrase pairs read; " 
                     << num_filtered << " filtered due to low counts" << endl;

   // finish computing expected counts

   if (exp_count_pruning) {
      if (verbose) cerr << "computing expected counts" << endl;
      for (PhraseTableGen<Uint>::iterator p = pt.begin(); p != pt.end(); ++p) {
         PhraseInfo& pi = infos[p.getJointFreq()];
         Uint cs = src_counts[p.getPhraseIndex(1)];
         Uint ct = tgt_counts[p.getPhraseIndex(2)];
         pi.vit_logpr = (ct * pi.logpr + cs * pi.vit_logpr) / 2.0;
      }
   }
   
   // find Viterbi decomposition of each phrase pair

   if (computing_scores) {
      if (verbose) cerr << "finding Viterbi decompositions" << endl;
      Uint num_decomposed = 0;
      // index -> phrase list - because omp needs integer loop
      vector< PhraseTableGen<Uint>::iterator > iters(infos.size());
      for (PhraseTableGen<Uint>::iterator p = pt.begin(); p != pt.end(); ++p)
         iters[p.getJointFreq()] = p;
      #pragma omp parallel
      {
         if (verbose && omp_get_thread_num() == 0) {
            cerr << "parallelizing - " << omp_get_num_threads() << " threads\n";
            if (infos.size() > 50) cerr << "progress (/50 dots): ";
         }
         #pragma omp for schedule(dynamic)
         for (Uint i = 0; i < infos.size(); ++i) {
            if (computeViterbiScore(pt, infos, iters[i], infos[i]))
               #pragma omp critical
               ++num_decomposed;
            if (verbose && infos.size() > 50 && i % (infos.size() / 50) == 0) 
               cerr << '.';
         }
      }
      if (verbose) {
         if (infos.size() > 50) cerr << endl;
         cerr << num_decomposed << "/" << infos.size() 
              << " phrases successfully decomposed (max hyps per src pos = "
              << max_hyps_used << ")" << endl;
      }
   } else
      if (verbose) 
         cerr << "skipping Viterbi step - " << 
            (count_pruning ? "doing count-based pruning" : 
             exp_count_pruning ? "doing expected count-based pruning" : 
             "using read-in scores") << endl;

   // determine pruning threshold

   if (verbose) cerr << "determining pruning threshold" << endl;
   float thresh = prune_val;  // correct if prune_type == Thresh
   Uint num_to_keep = 0;
   Uint num_to_keep_at_thresh = 0;
   if (prune_type == Abs || prune_type == Rel) {
      if (prune_type == Rel) prune_val *= infos.size() / 100.0;
      num_to_keep = prune_val; // use an integer
      vector<Uint> heap;
      heap.reserve(num_to_keep+1);
      ComparePhraseInfos cmp(infos);
      for (Uint i = 0; i < infos.size(); ++i) {
         heap.push_back(i);
         push_heap(heap.begin(), heap.end(), cmp);
         if (heap.size() > num_to_keep) {
            pop_heap(heap.begin(), heap.end(), cmp);
            heap.pop_back();
         }
      }
      thresh = heap.size() ? infos[heap[0]].score() : 0.0;
      for (Uint i = 0; i < heap.size(); ++i) 
         if (infos[heap[i]].score() == thresh)
            ++num_to_keep_at_thresh;
   }
   if (verbose) 
      cerr << "threshold = " << thresh << ": retaining " 
           << num_to_keep-num_to_keep_at_thresh << " pairs > thresh, and first " 
           << num_to_keep_at_thresh << " pairs at thresh" << endl;

   // write pruned table
   
   if (verbose) cerr << "writing output" << endl;
   string sp, tp;
   string sep = " " + PhraseTableBase::psep + " ";
   Uint nwritten = 0, nwritten_at_thresh = 0;
   if (low_mem) {  // re-read table and write selected lines
      istr.close(); 
      istr.open(infile);
      assert(istr.good());
      for (Uint i = 0; i < infos.size() && nwritten < num_to_keep; ++i) {
         if (!getline(istr, line)) assert(false);
         float s = infos[i].score();
         if (s > thresh || (s == thresh && nwritten_at_thresh < num_to_keep_at_thresh)) {
            if (read_scores) getScore(line, NULL); // remove final score field
            cout << line;
            if (write_scores) cout << ' ' << s;
            cout << endl;
            ++nwritten;
            if (s == thresh) ++nwritten_at_thresh;
         }
      }
   } else {   // write using in-memory data
      PhraseTableGen<Uint>::iterator p;
      for (p = pt.begin(); p != pt.end() && nwritten < num_to_keep; ++p) {
         Uint i = p.getJointFreq();
         float s = infos[i].score();
         if (s > thresh || (s == thresh && nwritten_at_thresh < num_to_keep_at_thresh)) {
            cout << p.getPhrase(1, sp) << sep << p.getPhrase(2, tp) << sep 
                 << infos[i].vals;
            if (write_scores) cout << ' ' << s;
            cout << endl;
            ++nwritten;
            if (s == thresh) ++nwritten_at_thresh;
         }
      }
   }
   if (verbose) cerr << nwritten << " phrase pairs written" << endl;
}

// Get score in final field on line, and truncate field to avoid confusing
// normal phrase table processing.

void getScore(string& line, float* score)
{
   Uint p = line.rfind(" ");
   if (p == string::npos)
      error(ETFatal, "expecting score in final field with -r option");
   if (score) *score = conv<float>(line.substr(p));
   line.erase(p);
}

Uint getJointCount(PhraseTableBase::ToksIter a, PhraseTableBase::ToksIter f,
                   const string& line)
{
   for (; a < f; ++a)
      if (isPrefix("c=", *a)) {
         float x = conv<float>((*a)+2);
         return static_cast<Uint>(x);
      }
   static Uint warn_count = 0;
   if (warn_count < 5)
      error(ETWarn, "no joint count (c=) field - using 1: %s", line.c_str());
   else if (warn_count == 5)
      error(ETWarn, "turning off further joint-count warnings");
   warn_count++;
   return 1;
}

/*-----------------------------------------------------------------------------
 Viterbi decoding and helpers.
 ----------------------------------------------------------------------------*/

typedef PhraseTableBase::IndIter IndIter;
typedef PhraseTableGen<Uint>::QueryObj QueryObj;

// Represent a hypothesis that covers a prefix of the source phrase (left
// implicit), with a set of target positions and a log prob.

struct Hypothesis {

   bitset<MAX_PHRASE_LEN> tcov;  // target coverage vector
   float logpr;     // log probability

   Hypothesis() : logpr(0) {}
   void dump() {
      cerr << "tcov=";
      // do this 'manually' in order to print left-to-right
      for (Uint i = 0; i < tcov.size(); ++i) cerr << (tcov[i] ? '1' : '0');
      cerr << ", logpr=" << logpr << endl;
   }

   // Check whether this hyp can be extended via the given target range. If so,
   // add its extension to the given set of hyps, or adjust its score if it's
   // already there. Return true if extension was possible.
   bool extend(Uint b, Uint e, float logpr, vector<Hypothesis>& hyps)
   {
      for (Uint i = b; i < e; ++i)
         if (tcov[i]) return false;
      Hypothesis h(*this);
      for (Uint i = b; i < e; ++i)
         h.tcov[i] = 1;
      h.logpr += logpr;
      for (Uint i = 0; i < hyps.size(); ++i)
         if (hyps[i].tcov == h.tcov) {
            if (h.logpr > hyps[i].logpr) 
               hyps[i].logpr = h.logpr;
            return true;
         }
      hyps.push_back(h);  // completely new
      return true;
   }
};

void dumpPhrase(PhraseTableUint::iterator p);
void dumpHypStack(Uint spos, vector<Hypothesis>& hyps);
void dumpTgtRanges(vector<Uint>& tp, vector<QueryObj>& qos);
void dumpTrans(Uint sbeg, Uint send, vector<Uint>& tp, 
               vector<QueryObj>& qos, vector<PhraseInfo>& infos);
void dumpFinalHyps(vector<Hypothesis>& hyps);
bool computeITGViterbiScore(PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                            PhraseTableGen<Uint>::iterator p, PhraseInfo& pi);

/**
 * Compute the Viterbi score for a given phrase pair within a phrase table.
 * @param table phrase table, with freq field used for unique index
 * @param infos index -> phrase scores (and other info)
 * @param p current phrase pair
 * @param pi its associated info struct; pi.vit_logpr will be set to Viterbi
 * score on return, if decomposition found
 * @return true if valid decomposition found
 */
bool computeViterbiScore(PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                         PhraseTableGen<Uint>::iterator p, PhraseInfo& pi)
{
   if (debug) dumpPhrase(p);

   // check for 1-many condition and itg threshold
   Uint minlen = min(p.getPhraseLength(1), p.getPhraseLength(2));
   if (minlen == 1)
      return false;
   if (itg_thresh && minlen >= itg_thresh)
      return computeITGViterbiScore(table, infos, p, pi);

   // data structures
   vector<Uint> sp, tp;  // tokenized & indexed src,tgt phrases
   p.getPhrase(1, sp);
   p.getPhrase(2, tp);
   vector< vector<Hypothesis> > hyps(sp.size()+1); // src pos -> xtendable hyps
   hyps[0].push_back(Hypothesis());
   vector<QueryObj> qos; // tgt ranges to query

   // try to extend all hyps that end immediately before each src pos
   for (Uint i = 0; i < sp.size(); ++i) {
      if (debug) dumpHypStack(i, hyps[i]);
      if (hyps[i].size() > max_hyps_used) max_hyps_used = hyps[i].size();

      // enumerate tgt ranges compatible with at least one hyp
      bitset<MAX_PHRASE_LEN> tcov(string(MAX_PHRASE_LEN, '1'));
      for (Uint j = 0; j < hyps[i].size(); ++j)
         tcov &= hyps[i][j].tcov;
      qos.clear();
      for (Uint j = 0; j < tp.size(); ++j) {
         if (!tcov[j]) {
            for (Uint k = j; k < tp.size() && !tcov[k]; ++k)
               qos.push_back(QueryObj(tp.begin()+j, tp.begin()+k+1));
         }
      }
      if (debug) dumpTgtRanges(tp, qos);

      // extend each hyp at current src pos in all valid ways
      for (Uint j = i; j < sp.size(); ++j) {  // all src phrases starting here
         if (j-i+1 == sp.size()) continue;  // disallow whole src phrase
         if (!table.query(sp.begin()+i, sp.begin()+j+1, qos)) // get trans
            continue; // src not found
         if (debug) dumpTrans(i, j+1, tp, qos, infos);
         for (Uint k = 0; k < qos.size(); ++k) {  // all matching translations
            if (!qos[k].found) continue;
            for (Uint h = 0; h < hyps[i].size(); ++h)   // all hyps to be extended
               hyps[i][h].extend(qos[k].beg2-tp.begin(), qos[k].end2-tp.begin(),
                                 infos[qos[k].val].logpr, hyps[j+1]);
         }
      }
   }
   // check final hyps for completeness
   if (debug) dumpFinalHyps(hyps[sp.size()]);
   for (Uint j = 0; j < hyps[sp.size()].size(); ++j)
      if (hyps[sp.size()][j].tcov.count() == tp.size()) {
         pi.vit_logpr = hyps[sp.size()][j].logpr;
         return true;
      }
   return false;
}

ostream& dumpRange(Uint b, Uint e) 
{
   cerr << "[" << b << "," << e << ")";
   return cerr;
}

ostream& dumpRange(vector<Uint>& p, IndIter b, IndIter e) {
   return dumpRange(b-p.begin(), e-p.begin());
}

void dumpPhrase(PhraseTableGen<Uint>::iterator p)
{
   string ss, ts;
   p.getPhrase(1, ss);
   p.getPhrase(2, ts);
   cerr << "* scoring phrase: " << ss << " ||| " << ts << endl;
}

void dumpHypStack(Uint spos, vector<Hypothesis>& hyps)
{
   cerr << "extending hyps at src position " << spos << ": " << endl;
   for (Uint i = 0; i < hyps.size(); ++i)
      hyps[i].dump();
}

void dumpTgtRanges(vector<Uint>& tp, vector<QueryObj>& qos)
{
   cerr << "considering tgt ranges: ";
   for (Uint i = 0; i < qos.size(); ++i)
      dumpRange(tp, qos[i].beg2, qos[i].end2) << ' ';
   cerr << endl;
}
 
void dumpTrans(Uint sbeg, Uint send, vector<Uint>& tp, 
               vector<QueryObj>& qos, vector<PhraseInfo>& infos)
{
   cerr << "translations (tgt ranges) for src range: ";
   dumpRange(sbeg, send) << ": " << endl;
   for (Uint i = 0; i < qos.size(); ++i) {
      if (qos[i].found)
         dumpRange(tp, qos[i].beg2, qos[i].end2) << " logpr=" 
            << infos[qos[i].val].logpr << endl;
   }
}

void dumpFinalHyps(vector<Hypothesis>& hyps)
{
   cerr << "final hyps (only one should have full tgt coverage):" << endl;
   for (Uint i = 0; i < hyps.size(); ++i)
      hyps[i].dump();
   if (hyps.size() == 0) cerr << "[none]" << endl;
}

/*-----------------------------------------------------------------------------
 ITG Viterbi decoding and helpers. This is an approximation to true Viterbi
 decoding based on the ITG constraint: only recursive binary splits (straight
 or swapped) are considered.

 For speed and compatibility with multi-threaded execution, the implementation
 below assumes that the Viterbi log prob of any sub phrase pair is less than
 the pair's native log prob. Specifically, if a sub phrase pair is found in the
 phrase table, its log prob is used in place of its true Viterbi probability.
 Decomposing to find the latter would take more time; and assuming that it had
 already been found and stored in the vit_logpr field would break parallelism.

 The computeITGScoreFast() variant, invoked by the undocumented -f option, was
 intended to be faster. It isn't. Results are also slightly different from
 plain ITG, which they shouldn't be.
 ----------------------------------------------------------------------------*/

bool computeITGScore(const vector<Uint>& sp, const vector<Uint>& tp, 
                     Uint i, Uint j, Uint k, Uint l, bool top,
                     PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                     float& lp);
bool computeITGScoreFast(const vector<Uint>& sp, const vector<Uint>& tp, 
                         Uint i, Uint j, Uint k, Uint l,
                         PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                         float& lp);

/**
 * Compute the ITG approximation to Viterbi score for a given phrase pair
 * within a phrase table.
 * @param table phrase table, with freq field used for unique index
 * @param infos index -> phrase scores (and other info)
 * @param p current phrase pair
 * @param pi its associated info struct; pi.vit_logpr will be set to Viterbi
 * score on return, if decomposition found
 * @return true if valid decomposition found
 */
bool computeITGViterbiScore(PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                            PhraseTableGen<Uint>::iterator p, PhraseInfo& pi)
{
   vector<Uint> sp, tp;
   p.getPhrase(1, sp);
   p.getPhrase(2, tp);
   float lp;

   bool res = itg_fast ? 
      computeITGScoreFast(sp, tp, 0, sp.size(), 0, tp.size(), table, infos, lp) :
      computeITGScore(sp, tp, 0, sp.size(), 0, tp.size(), true, 
                      table, infos, lp);

   if (res) {
      if (debug) cerr << "ITG" << (itg_fast ? " fast " : " ")
                      << "decomp succeeded with logpr=" << lp << endl;
      pi.vit_logpr = lp;
      return true;
   }
   if (debug) cerr << "ITG decomp failed" << endl;
   return false;
}

bool computeITGScore(const vector<Uint>& sp, const vector<Uint>& tp, 
                     Uint i, Uint j, Uint k, Uint l, bool top,
                     PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                     float& lp)
{
   // if we're recursed, check whole-phrase solution
   Uint ind;
   if (!top &&
       table.exists(sp.begin()+i, sp.begin()+j, tp.begin()+k, tp.begin()+l, ind)) {
      lp = infos[ind].logpr;
      return true;
   }
   
   // find best pair-wise decomposition
   lp = -numeric_limits<float>::infinity();
   for (Uint s = i+1; s < j; ++s) {
      for (Uint t = k+1; t < l; ++t) {
         float lp1, lp2;
         if (computeITGScore(sp, tp, i, s, k, t, false, table, infos, lp1) &&
             computeITGScore(sp, tp, s, j, t, l, false, table, infos, lp2))
            lp = max(lp, lp1+lp2);
         if (computeITGScore(sp, tp, i, s, t, l, false, table, infos, lp1) &&
             computeITGScore(sp, tp, s, j, k, t, false, table, infos, lp2))
            lp = max(lp, lp1+lp2);
      }
   }
   return lp != -numeric_limits<float>::infinity();
}

// Sets lp to -inf on failure.

bool computeITGScoreFast(const vector<Uint>& sp, const vector<Uint>& tp, 
                         Uint i, Uint j, Uint k, Uint l,
                         PhraseTableGen<Uint>& table, vector<PhraseInfo>& infos,
                         float& lp)
{
   // construct queries for complementary target ranges
   Uint nts = l-k-1; // number of target split points
   vector<QueryObj> qos1(2 * nts); // hold results for left src split
   for (Uint t = k+1; t < l; ++t) {
      qos1[t-k-1] = QueryObj(tp.begin()+k, tp.begin()+t); // left tgt split
      qos1[t-k-1+nts] = QueryObj(tp.begin()+t, tp.begin()+l); // rg tgt split
   }
   vector<QueryObj> qos2(qos1.begin(), qos1.end()); // res for rt src split

   // query translations for complementary source ranges
   lp = -numeric_limits<float>::infinity();
   float lp1, lp2;
   for (Uint s = i+1; s < j; ++s) {
      table.query(sp.begin()+i, sp.begin()+s, qos1);
      table.query(sp.begin()+s, sp.begin()+j, qos2);

      for (Uint t = k+1; t < l; ++t) {
         Uint u = t-k-1, v = t-k-1+nts;

         // straight orientation
         lp1 = qos1[u].found ? infos[qos1[u].val].logpr : (s-i > 1 && t-k > 1) ?
            computeITGScoreFast(sp, tp, i, s, k, t, table, infos, lp1), lp1 : 
            -numeric_limits<float>::infinity();
         lp2 = (lp1 == -numeric_limits<float>::infinity()) ? lp1 :
            qos2[v].found ? infos[qos2[v].val].logpr : (j-s > 1 && l-t > 1) ?
            computeITGScoreFast(sp, tp, s, j, t, l, table, infos, lp2), lp2 :
            -numeric_limits<float>::infinity();
         lp = max(lp, lp1+lp2);

         // swapped orientation
         lp1 = qos1[v].found ? infos[qos1[v].val].logpr : (s-i > 1 && l-t > 1) ?
            computeITGScoreFast(sp, tp, i, s, t, l, table, infos, lp1), lp1 :
            -numeric_limits<float>::infinity();
         lp2 = (lp1 == -numeric_limits<float>::infinity()) ? lp1 :
            qos2[u].found ? infos[qos2[u].val].logpr : (j-s > 1 && t-k > 1) ?
            computeITGScoreFast(sp, tp, s, j, k, t, table, infos, lp2), lp2 :
            -numeric_limits<float>::infinity();
         lp = max(lp, lp1+lp2);
      }
   }
   return lp != -numeric_limits<float>::infinity();
}


/*-----------------------------------------------------------------------------
 arg processing
 ----------------------------------------------------------------------------*/

void getArgs(int argc, char* argv[])
{
   const char* switches[] = {"v", "d", "o", "r", "j", "e", "ec:", "l", "s:", 
                             "pr:", "pa:", "pt:", "c:", "m:", "i:", "f"};
   ArgReader arg_reader(ARRAY_SIZE(switches), switches, 0, 1, help_message);
   arg_reader.read(argc-1, argv+1);

   arg_reader.testAndSet("v", verbose);
   arg_reader.testAndSet("d", debug);
   arg_reader.testAndSet("o", write_scores);
   arg_reader.testAndSet("r", read_scores);
   arg_reader.testAndSet("j", count_pruning);
   arg_reader.testAndSet("e", exp_count_pruning);
   arg_reader.testAndSet("ec", exp_count_col);
   arg_reader.testAndSet("l", low_mem);
   arg_reader.testAndSet("s", score_col);
   arg_reader.testAndSet("pr", prune_val);
   arg_reader.testAndSet("pa", prune_val);
   arg_reader.testAndSet("pt", prune_val);
   arg_reader.testAndSet("c", default_vit_logpr);
   arg_reader.testAndSet("m", min_count);
   arg_reader.testAndSet("i", itg_thresh);
   arg_reader.testAndSet("f", itg_fast);

   if (arg_reader.getSwitch("pr")) prune_type = Rel;
   else if (arg_reader.getSwitch("pa")) prune_type = Abs;
   else if (arg_reader.getSwitch("pt")) prune_type = Thresh;
      
   arg_reader.testAndSet(0, "infile", infile);

   if (count_pruning && exp_count_pruning)
      error(ETFatal, "switches -j and -e are not compatible");

   if (low_mem) {
      if (min_count) error(ETFatal, "switches -l and -m are not compatible");
      if (infile == "-") error(ETFatal, "input must be a file to use the -l switch");
   }
}
