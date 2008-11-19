/**
 * @author George Foster, based on "rescore" by Aaron Tikuisis, plus many others...
 * @file rescore_train.cc
 * @brief Program rescore_train which finds the best weights
 * for a set of feature functions given sources, references and nbest lists.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "rescore_train.h"
#include "boostDef.h"
#include "featurefunction_set.h"
#include "powell.h"
#include "rescore_io.h"
#include "fileReader.h"
#include "gfstats.h"
#include "basic_data_structure.h"
#include "referencesReader.h"
#include "printCopyright.h"
#include "progress.h"
#include "bleu.h"
#include "PERstats.h"
#include "WERstats.h"
#ifdef _OPENMP
#include <omp.h>
#endif


using namespace Portage;
using namespace std;
using namespace rescore_train;


/// history datum.
template<class ScoreStats>
struct history_datum {
   double score;   ///< a score.
   uVector wts;    ///< associated weights.
   /// Constructor.
   /// @param score  score
   /// @param wts    associated weigths.
   history_datum(double score, const uVector& wts)
   : score(score)
   , wts(wts)
   {}

   /**
    * Defines ordering (greater) for two datum.
    * @param other  right-side operand
    * @return Returns true if score > other.score.
    */
   bool operator>(const history_datum& other) const {
      return score > other.score;
   }
};

template<class ScoreStats>
double runPowell(const vector<uMatrix>& vH,
                 const vector< vector<ScoreStats> >& bleu,
                 const vector<uVector>& init_wts,
                 const FeatureFunctionSet& ffset,
                 const rescore_train::ARG& arg, // oi, vy, vy?
                 uVector& best_wts,
                 vector< history_datum<ScoreStats> >&  history);


/**
 * Defines how to print an history_datum.
 * @param out  where to output the history_datum.
 * @param o    history_datum to output
 * @return Returns out + history_datum.
 */
template<class ScoreStats>
ostream& operator<<(ostream& out, const history_datum<ScoreStats>& o) {
   out << ScoreStats::name() << " score: ";
   out << ScoreStats::convertToDisplay(o.score) << " ";
   copy(o.wts.begin(), o.wts.end(), ostream_iterator<double>(out, " "));
   out << endl;
   return out;
}


/**
 * Calculates the BLEU stats for a given nbest list.
 * @param scores [out] BLEU stats for each hypotheses in the nbest
 * @param nbest  [in] hypotheses for a source sentence.
 * @param refs   [in] references for a source sentence.
 * @param arg    [in] user's arguments.
 */
void computeScore(vector<BLEUstats>& scores, const Nbest& nbest, const References& refs, const ARG& arg)
{
   computeBLEUArrayRow(scores, nbest, refs, numeric_limits<Uint>::max(), arg.sm);
}

/**
 * Calculates the PER stats for a given nbest list.
 * @param scores [out] PER stats for each hypotheses in the nbest
 * @param nbest  [in] hypotheses for a source sentence.
 * @param refs   [in] references for a source sentence.
 * @param arg    [in] user's arguments.
 */
void computeScore(vector<PERstats>& scores, const Nbest& nbest, const References& refs, const ARG& arg)
{
   scores.clear();
   scores.reserve(nbest.size());
   for (Uint i(0); i<nbest.size(); ++i) {
      scores.push_back(PERstats(nbest[i], refs));
   }
}

/**
 * Calculates the WER stats for a given nbest list.
 * @param scores [out] WER stats for each hypotheses in the nbest
 * @param nbest  [in] hypotheses for a source sentence.
 * @param refs   [in] references for a source sentence.
 * @param arg    [in] user's arguments.
 */
void computeScore(vector<WERstats>& scores, const Nbest& nbest, const References& refs, const ARG& arg)
{
   scores.clear();
   scores.reserve(nbest.size());
   for (Uint i(0); i<nbest.size(); ++i) {
      scores.push_back(WERstats(nbest[i], refs));
   }
}


/// Forward declaration.
template<class ScoreStats>
void train(const ARG& arg);


////////////////////////////////////////////////////////////////////////////////
// MAIN
int main(int argc, const char* const argv[])
{
   printCopyright(2004, "rescore_train");
#ifdef _OPENMP
   cerr << "Compiled openmp" << endl;
#pragma omp parallel
#pragma omp master
   fprintf(stderr, "Using %d threads.\nSet OMP_NUM_THREADS=N to change to N threads.\n",
           omp_get_num_threads());
#endif

   const ARG arg(argc, argv);

   switch (arg.training_type) {
   case ARG::BLEU:
      BLEUstats::setDefaultSmoothing(arg.sm);
      BLEUstats::setMaxNgrams(arg.maxNgrams);
      BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);

      train<BLEUstats>(arg);
      break;
   case ARG::PER:
      train<PERstats>(arg);
      break;
   case ARG::WER:
      train<WERstats>(arg);
      break;
   default:
      cerr << "Not implemented yet" << endl;
   }
}

// Apply powell using either BLEU/PER/WER.
template<class ScoreStats>
void train(const ARG& arg)
{
   LOG_VERBOSE2(verboseLogger, "Training using %s", ScoreStats::name());
   cerr << "SCORE TYPE: " << ScoreStats::name() << endl;  // SAM DEBUG

   // Start of loading
   time_t start = time(NULL);             // time

   LOG_VERBOSE2(verboseLogger, "Reading source sentences");
   Sentences  sources;
   const Uint S = RescoreIO::readSource(arg.src_file, sources);
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());

   LOG_VERBOSE2(verboseLogger, "Reading Feature Function Set");
   FeatureFunctionSet ffset(true, arg.seed);
   // When we want to do the final clean-up, we don't use the nullDeleter.
   const Uint M = ffset.read(arg.model_in, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic, !arg.do_clean_up);
   ffset.createTgtVocab(sources, FileReader::create<Translation>(arg.nbest_file, arg.K));

   LOG_VERBOSE2(verboseLogger, "Init ff matrix with %d source sentences", S);
   ffset.initFFMatrix(sources);

   LOG_VERBOSE2(verboseLogger, "Creating references reader");
   referencesReader  rReader(arg.refs_file);
   const Uint R(rReader.getR());

   LOG_VERBOSE2(verboseLogger, "Rescoring with S = %d, K = %d, R = %d, M = %d", S, arg.K, R, M);
   vector<uMatrix>  vH(S);
   vector< vector<ScoreStats> >  scores(S);

   ifstream astr;
   const bool bNeedsAlignment = ffset.requires() & FF_NEEDS_ALIGNMENT;
   if (bNeedsAlignment) {
      if (arg.alignment_file.empty())
         error(ETFatal, "At least one of feature function requires the alignment");
      LOG_VERBOSE2(verboseLogger, "Reading alignments from %s", arg.alignment_file.c_str());
      astr.open(arg.alignment_file.c_str());
      if (!astr) error(ETFatal, "unable to open alignment file %s", arg.alignment_file.c_str());
   }

   vector< uVector > powellWeightsIn;
   if (arg.weight_infile != "") {
      LOG_VERBOSE2(verboseLogger, "Reading Powell weights from %s", arg.weight_infile.c_str());
      ifstream wstr(arg.weight_infile.c_str());
      if (!wstr) error(ETFatal, "unable to open Powell weight file %s", arg.weight_infile.c_str());

      string line;
      Uint num_lines = 0;
      while (getline(wstr, line) &&
             (arg.weight_infile_nl == 0 || num_lines++ < arg.weight_infile_nl)) {
         vector<string> toks;
         split(line, toks);
         const Uint beg = toks.size() > 0 && toks[1] == "score:" ? 3 : 0;
         if (toks.size() - beg != M)
            error(ETFatal, "wrong number of features in weight file %s: found %i, expected %i",
                  arg.weight_infile.c_str(), toks.size() - beg, M);
         powellWeightsIn.push_back(uVector(M));
         for (Uint i = 0; i < M; ++i)
            powellWeightsIn.back()[i] = conv<double>(toks[beg+i]);
      }
   }


   // Read and process the N-bests file of candidate target sentences
   LOG_VERBOSE2(verboseLogger, "Processing nbests lists (computing feature values and %s scores)", ScoreStats::name());
   NbestReader  pfr(FileReader::create<Translation>(arg.nbest_file, arg.K));
   Uint s(0);
   Progress progress(S, arg.bVerbose); // Must be last statement before for loop => pretty display
   progress.displayBar();

   for (; pfr->pollable(); ++s)
   {
      // READING NBEST
      Nbest  nbest;
      pfr->poll(nbest);
      Uint K(nbest.size());

      // READING REFERENCES
      References  refs(R);
      rReader.poll(refs);

      // READING ALIGNMENT
      vector<Alignment> alignments(K);
      Uint k(0);
      for (; bNeedsAlignment && k < K && alignments[k].read(astr); ++k)
      {
         nbest[k].alignment = &alignments[k];
      }
      if (bNeedsAlignment && (k != K ))
         error(ETFatal, "unexpected end of nbests file after %d lines (expected %dx%d=%d lines)", s*K+k, S, K, S*K);

      LOG_VERBOSE5(verboseLogger, "computing ff matrix for s=%d with K=%d and M=%d", s, K, M);
      ffset.computeFFMatrix(vH[s], s, nbest);
      K = nbest.size();  // IMPORTANT K might change if computeFFMatrix detects empty lines

      LOG_VERBOSE5(verboseLogger, "computing a score row for s=%d with K=%d, R=%d", s, K, R);
      computeScore(scores[s], nbest, refs, arg);

      progress.step();
   } // for

   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);
   rReader.integrityCheck();
   if (!ffset.complete())
      error(ETFatal, "It seems there is still some ffvals but we've exhausted the NBests.");


   ////////////////////////////////////////////////////////////////////////////////
   // Free memory we no longer need
   astr.close();

   cerr << "Done loading in " << (Uint)(time(NULL) - start) << " seconds" << endl;

   start = time(NULL);
   LOG_VERBOSE2(verboseLogger, "Running Powell's algorithm");
   uVector best_wts(M);
   vector< history_datum<ScoreStats> >  history; // list of -score,wts vectors
   const double best_score = runPowell<ScoreStats>(
      vH, scores, powellWeightsIn, ffset,
      arg, best_wts, history);
   cerr << "Best weight vector found in " << (Uint)(time(NULL) - start) << " seconds" << endl;

   // Normalize and write weights

   if (best_score == -INFINITY)
      error(ETWarn, "No global maximum found! Setting all weights to 1");

   if (arg.bNormalize)
      best_wts /= ublas::norm_inf(best_wts);
   for (Uint m = 0; m < M; ++m)
      if (m >= arg.flor && best_wts(m) < 0.0)
         best_wts(m) = 0.0;
   ffset.setWeights(best_wts);

   if (arg.bVerbose) {
      cerr << "Best score: " << ScoreStats::convertToDisplay(best_score) << endl;
      cerr << "Using normalized wts=" << best_wts << endl;
   }

   if (!arg.weight_outfile.empty()) {
      LOG_VERBOSE2(verboseLogger, "Writing best powell weights to file");
      sort(history.begin(), history.end(), greater< history_datum<ScoreStats> >());
      ofstream woutstr(arg.weight_outfile.c_str(), ios_base::app);
      copy(history.begin(), history.end(), ostream_iterator< history_datum<ScoreStats> >(woutstr));
      woutstr.flush();
      woutstr.close();
   }

   LOG_VERBOSE2(verboseLogger, "Writing model to file");
   ffset.write(arg.model_out);
}

/**
 * Run Powell's algorithm with different start parameters until a global
 * maximum appears to be found.
 * @param vH vector of S KxM matrices of feature values
 * @param bleu vector of S vectors of K hypothesis scores
 * @param init_wts starting wts for initial iterations
 * @param ffset used only for random wt generation, honest
 * @param arg user-specified arguments
 * @param best_wts place to put the best wts. Assumed to be initialized to the
 * proper size.
 * @param history on will be set to a list of -score,wts vectors for each iter
 * @return best score found
 */
template<class ScoreStats>
double runPowell(const vector<uMatrix>& vH,
                 const vector< vector<ScoreStats> >& scores,
                 const vector<uVector>& init_wts,
                 const FeatureFunctionSet& ffset,
                 const rescore_train::ARG& arg, // oi, vy, vy?
                 uVector& best_wts,
                 vector< history_datum<ScoreStats> >&  history)
{
   fill(best_wts.begin(), best_wts.end(), 1.0f);
   double best_score(-INFINITY);
   double extend_thresh(0.0);   // score threshold for extending total_runs
   Uint num_runs(0);
   Uint total_runs = arg.num_powell_runs == 0 ?
      (rescore_train::NUM_INIT_RUNS+2) :  // +2 for backward compatibility (!)
      arg.num_powell_runs;
   vector<double> score_history; // list of bleu scores, for approx-expect stopping

   Uint M = best_wts.size();

   Powell<ScoreStats> powell(vH, scores);
   while (num_runs < total_runs) {

      uVector wts(M);
      if (num_runs < init_wts.size())
         wts = init_wts[num_runs];
      else
         ffset.getRandomWeights(wts);

      if (arg.bVerbose) {
         cerr << "Calling Powell with initial wts=" << wts << endl;
      }

      int iter = 0;
      double score = 0.0;
      time_t powell_start = time(NULL);             // time
      powell(wts, POWELL_TOLERANCE, iter, score);
      ++num_runs;

      if (arg.bVerbose) {
         cerr << "Powell returned wts=" << wts << endl;
         fprintf(stderr, "Score: %f in %d seconds\n", ScoreStats::convertToDisplay(score), (Uint)(time(NULL)- powell_start));
      }

      history.push_back(history_datum<ScoreStats>(score, wts));
      score_history.push_back(ScoreStats::convertToDisplay(score));

      if (score > best_score) {
         best_wts = wts;
         best_score = score;
      }
      if (!arg.approx_expect && arg.num_powell_runs == 0 && ScoreStats::convertToDisplay(score) > extend_thresh) {
         // 'normal' stopping criterion
         total_runs = max(num_runs+rescore_train::NUM_INIT_RUNS+2, 2 * num_runs); // bizarre for bkw compat
         extend_thresh = ScoreStats::convertToDisplay(score) + SCORETOL;
      } else if (arg.approx_expect && num_runs > rescore_train::NUM_INIT_RUNS+2) {
         // approx-expect stopping
         const double m = mean(score_history.begin(), score_history.end());
         const double s = sdev(score_history.begin(), score_history.end(), m);
         const Uint expected_iters = Uint(1.0 / (1.0 - normalCDF(ScoreStats::convertToDisplay(best_score), m, s)));
         if (num_runs + expected_iters > total_runs)
            break;
      }
   }
   
   return best_score;
}
