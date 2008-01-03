/**
 * @author George Foster, based on "rescore" by Aaron Tikuisis, plus many others...
 * @file rescore_train.cc  Program rescore_train which finds the best weights
 * for a set of feature functions given sources, references and nbest lists.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include <rescore_train.h>
#include <boostDef.h>
#include <featurefunction.h>
#include <powell.h>
#include <rescore_io.h>
#include <fileReader.h>
#include <gfstats.h>
#include <bleu.h>
#include <basic_data_structure.h>
#include <referencesReader.h>
#include <printCopyright.h>
#include <progress.h>
#include <iostream>
#include <typeinfo>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif


using namespace Portage;
using namespace std;


////////////////////////////////////////////////////////////////////////////////
// MAIN
int main(const Uint argc, const char* const argv[])
{
   printCopyright(2004, "rescore_train");
#ifdef _OPENMP
   cerr << "Compiled openmp" << endl;
#pragma omp parallel
#pragma omp master
   fprintf(stderr, "Using %d threads.\nSet OMP_NUM_THREADS=N to change to N threads.\n",
           omp_get_num_threads());
#endif

   using namespace rescore_train;
   const ARG arg(argc, argv);

   BLEUstats::setMaxNgrams(arg.maxNgrams);
   BLEUstats::setMaxNgramsScore(arg.maxNgramsScore);

   // Start of loading
   time_t start = time(NULL);             // time

   LOG_VERBOSE2(verboseLogger, "Reading source sentences");
   Sentences  sources;
   const Uint S = RescoreIO::readSource(arg.src_file, sources);
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());

   LOG_VERBOSE2(verboseLogger, "Reading Feature Functions Set");
   FeatureFunctionSet ffset(true, arg.seed);
   const Uint M = ffset.read(arg.model_in, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic);
   ffset.createTgtVocab(sources, FileReader::create<Translation>(arg.nbest_file, arg.K));

   LOG_VERBOSE2(verboseLogger, "Init ff matrix with %d source sentences", S);
   ffset.initFFMatrix(sources);

   LOG_VERBOSE2(verboseLogger, "Creating references reader");
   referencesReader  rReader(arg.refs_file);
   const Uint R(rReader.getR());

   LOG_VERBOSE2(verboseLogger, "Rescoring with S = %d, K = %d, R = %d, M = %d", S, arg.K, R, M);
   vector<uMatrix>  vH(S);
   MatrixBLEUstats  bleu(S);

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
      while (getline(wstr, line) && num_lines++ < arg.weight_infile_nl) {
         vector<string> toks;
         split(line, toks);
         Uint beg = toks.size() > 0 && toks[0] == "BLEU" ? 3 : 0;
         if (toks.size() - beg != M)
            error(ETFatal, "wrong number of features in weight file %s: found %i, expected %i",
                  arg.weight_infile.c_str(), toks.size() - beg, M);
         powellWeightsIn.push_back(uVector(M));
         for (Uint i = 0; i < M; ++i) 
            powellWeightsIn.back()[i] = conv<double>(toks[beg+i]);
      }
   }


   // Read and process the N-bests file of candidate target sentences
   LOG_VERBOSE2(verboseLogger, "Processing nbests lists (computing feature values and BLEU scores)");
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

      LOG_VERBOSE5(verboseLogger, "computing a bleu row for s=%d with K=%d, R=%d", s, K, R);
      computeBLEUArrayRow(bleu[s], nbest, refs, numeric_limits<Uint>::max(), arg.sm);

      progress.step();
   } // for

   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);
   rReader.integrityCheck();
   if (!ffset.complete())
      error(ETFatal, "It seems there is still some ffvals but we've exhausted the NBests.");


   ////////////////////////////////////////////////////////////////////////////////
   // Free memory we no longer need
   astr.close();

   // How long was it to load.
   cerr << "Done loading in " << (Uint)(time(NULL) - start) << " seconds" << endl;


   // Start timer for powell
   start = time(NULL);             // time

   LOG_VERBOSE2(verboseLogger, "Running Powell's algorithm");

   uVector best_wts(M);
   fill(best_wts.begin(), best_wts.end(), 1.0f);
   double best_score(-INFINITY);
   double extend_thresh(0.0);   // bleu threshold for extending total_runs
   Uint num_runs(0);
   Uint total_runs = arg.num_powell_runs == 0 ? 
      (NUM_INIT_RUNS+2) :  // +2 for backward compatibility (!)
      arg.num_powell_runs;
   vector< vector<double> >  history; // list of -score,wts vectors
   vector<double> bleu_history; // list of bleu scores, for approx-expect stopping

   // Run Powell's algorithm with different start parameters until a global
   // maximum appears to be found.
   Powell<BLEUstats> powell(vH, bleu);
   while (num_runs < total_runs) {

      uVector wts(M);
      if (num_runs < powellWeightsIn.size())
         wts = powellWeightsIn[num_runs];
      else
         ffset.getRandomWeights(wts);

      if (arg.bVerbose) cerr << "Calling Powell with initial wts=" << wts << endl;

      uMatrix xi(M, M);
      xi = uIdentityMatrix(M);
      int iter = 0;
      double fret = 0.0;
      time_t powell_start = time(NULL);             // time
      powell(wts, xi, FTOL, iter, fret);
      ++num_runs;

      if (arg.bVerbose) {
         cerr << "Powell returned wts=" << wts << endl;
         fprintf(stderr, "Score: %f <=> %f in %d seconds\n", fret, exp(fret), (Uint)(time(NULL)- powell_start));
      }

      history.push_back(vector<double>(1, -fret));
      history.back().resize(1+M);
      copy(wts.begin(), wts.end(), history.back().begin()+1);
      bleu_history.push_back(exp(fret));

      if (fret > best_score) {
         best_wts = wts;
         best_score = fret;
      }
      if (!arg.approx_expect && arg.num_powell_runs == 0 && exp(fret) > extend_thresh) {
         // normal stopping criterion
         total_runs = max(num_runs+NUM_INIT_RUNS+2, 2 * num_runs); // bizarre for bkw compat
         extend_thresh = exp(fret) + BLEUTOL;
      } else if (arg.approx_expect && num_runs > NUM_INIT_RUNS+2) {
         // approx-expect stopping
         const double m = mean(bleu_history.begin(), bleu_history.end());
         const double s = sdev(bleu_history.begin(), bleu_history.end(), m);
         const Uint expected_iters = Uint(1.0 / (1.0 - normalCDF(exp(best_score), m, s)));
         if (num_runs + expected_iters > total_runs)
            break;
      }
   }

   // How long it took to run powell
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
      cerr << "Best score: " << best_score << " <=> " << exp(best_score) << endl;
      cerr << "Using floored & normalized wts=" << best_wts << endl;
   }

   if (!arg.weight_outfile.empty()) {
      sort(history.begin(), history.end());
      ofstream woutstr;
      woutstr.open(arg.weight_outfile.c_str(), ios_base::app);
      LOG_VERBOSE2(verboseLogger, "Writing best powell weights to file");
      for (Uint i = 0;  i < history.size(); i++) {
         woutstr << "BLEU score: " << exp(-history[i][0]);
         for (Uint j = 1; j < history[i].size(); ++j)
            woutstr << " " << history[i][j];
         woutstr << endl;
      }
      woutstr.flush();
      woutstr.close();
   }

   LOG_VERBOSE2(verboseLogger, "Writing model to file");
   ffset.write(arg.model_out);
}
