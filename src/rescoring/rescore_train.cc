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
#include <bleu.h>
#include <basic_data_structure.h>
#include <exception_dump.h>
#include <referencesReader.h>
#include <printCopyright.h>
#include <progress.h>
#include <iostream>
#include <typeinfo>
#include <iostream>
#include <assert.h>


using namespace Portage;
using namespace std;


////////////////////////////////////////////////////////////////////////////////
// MAIN
int MAIN(argc, argv)
{
   printCopyright(2004, "rescore_train");
   using namespace rescore_train;
   ARG arg(argc, argv);

   LOG_VERBOSE2(verboseLogger, "Reading Feature Functions Set");
   FeatureFunctionSet ffset;
   const Uint M = ffset.read(arg.model_in, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic, false);

   LOG_VERBOSE2(verboseLogger, "Reading source sentences");
   Sentences  sources;
   const Uint S = RescoreIO::readSource(arg.src_file, sources);
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());

   LOG_VERBOSE2(verboseLogger, "Creating references reader");
   referencesReader  rReader(arg.refs_file);
   const Uint R(rReader.getR());

   LOG_VERBOSE2(verboseLogger, "Rescoring with S = %d, K = %d, R = %d, M = %d", S, arg.K, R, M);
   vector<uMatrix>  vH(S);
   MatrixBLEUstats  bleu(S);

   ifstream astr;
   if (arg.bReadAlignments) {
      LOG_VERBOSE2(verboseLogger, "Reading alignments from %s", arg.alignment_file.c_str());
      astr.open(arg.alignment_file.c_str());
      if (!astr) error(ETFatal, "unable to open alignment file %s", arg.alignment_file.c_str());
   }

   vector< uVector > powellWeightsIn, powellWeightsOut;
   ifstream wstr;
   if (arg.weight_infile != "") {
      LOG_VERBOSE2(verboseLogger, "Reading Powell weights from %s", arg.weight_infile.c_str());
      wstr.open(arg.weight_infile.c_str());
      if (!wstr) error(ETFatal, "unable to open Powell weight file %s", arg.weight_infile.c_str());

      string tmpsco;
      uVector weights(M);
      uint m=0;
      while (wstr >> tmpsco) {
         if (tmpsco=="BLEU") {
            wstr >> tmpsco; wstr >> tmpsco; wstr >> tmpsco;
         }
         weights(m) = atof(tmpsco.c_str());
         if (++m==M) {
            powellWeightsIn.push_back(weights);
            weights.clear();
            m=0;
         }
      }
      if (m > 0)
         error(ETFatal, "Error in Powell weight file %s, wrong number of weights: %i instead of zero after reading %i sets of feature weights!", arg.weight_infile.c_str(), m, powellWeightsIn.size());
      // powellWeightsIn
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
      for (; arg.bReadAlignments && k < K && alignments[k].read(astr); ++k)
      {
         nbest[k].alignment = &alignments[k];
      }
      if (arg.bReadAlignments && (k != K )) 
         error(ETFatal, "unexpected end of nbests file after %d lines (expected %dx%d=%d lines)", s*K+k, S, K, S*K);

      LOG_VERBOSE5(verboseLogger, "Init ff matrix for s=%d with K=%d", s, K);
      ffset.initFFMatrix(sources, K);

      LOG_VERBOSE5(verboseLogger, "computing ff matrix for s=%d with K=%d and M=%d", s, K, M);
      ffset.computeFFMatrix(vH[s], s, nbest);
      K = nbest.size();  // IMPORTANT K might change if computeFFMatrix detects empty lines

      LOG_VERBOSE5(verboseLogger, "computing a bleu row for s=%d with K=%d, R=%d", s, K, R);
      computeBLEUArrayRow(bleu[s], nbest, refs);

      progress.step();
   } // for

   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);
   rReader.integrityCheck();


   ////////////////////////////////////////////////////////////////////////////////
   // Free memory we no longer need
   astr.close();

   LOG_VERBOSE2(verboseLogger, "Running Powell's algorithm");

   uVector modelP(M);
   uVector p(M);
   uMatrix xi(M, M);

   int iter(0);
   double fret(0.0f);
   double bestScore(-INFINITY);
   vector<double> bestScores(NUM_INIT_RUNS, -INFINITY);
   Uint numStable(0);
   Uint totalRuns(0);

   RNG_gen   rg;
   RNG_dist  rd(0, 3);
   RNG_type  rng(rg, rd);

   while (true) {

      // Run Powell's algorithm with different start parameters until a global
      // maximum appears to be found.  If the number of runs in which the
      // current maximum hasn't changed is at least half the total number of
      // runs, and the total number of runs is at least NUM_INIT_RUNS, then
      // that's when we're done.  Since there are only finitely many possible
      // scores, this will definitely terminate.

     // for the first half of the runs, re-use the old weights from the 
     // previous iteration
     if (totalRuns < min(powellWeightsIn.size(), NUM_INIT_RUNS/2))
       for (Uint m(0); m<M; ++m) {
         p = powellWeightsIn[totalRuns];
       }
     else
       for (Uint m(0); m<M; ++m) {
         p(m) = rng();
       }
     
     if (arg.bVerbose) cerr << "Calling Powell with initial p=" << p << endl;
     
      xi = uIdentityMatrix(M);
      powell(p, xi, FTOL, iter, fret, vH, bleu);

      if (arg.bVerbose)
         cerr << "Powell returned p=" << p << endl
              << "Score: " << fret << " <=> " << exp(fret) << endl;

      ++totalRuns;

      // check if this is among the best scores
      if (fret > bestScores.back()) {
        for (int i=0; bestScores.begin()+i != bestScores.end(); i++) {
          if (*(bestScores.begin()+i) < fret ) {
            bestScores.insert(bestScores.begin()+i,fret);
            powellWeightsOut.insert(powellWeightsOut.begin()+i,p);
            break;
          }
        }
      }

      // if (fret > bestScore) {
      if (exp(fret) > exp(bestScore) + BLEUTOL) {
        numStable = 0;
        modelP = p;
        bestScore = fret;
      } else {
        ++numStable;
        if (numStable > NUM_INIT_RUNS && numStable * 2 >= totalRuns)
          {
            if (fret == -numeric_limits<double>::infinity())
              error(ETFatal, "Powell returned %g, maybe your files are bad!", fret);
            break;
          }
      }
   }

   if (arg.bNormalize)
   {
      modelP /= ublas::norm_inf(modelP);
   }

   uVector vTemp(modelP);
   for (Uint m(0); m<M; ++m) {
      if (m >= arg.flor && vTemp(m) < 0)
         vTemp(m) = 0;
   }
   ffset.setWeights(vTemp);

   if (arg.bVerbose) {
      cerr << "Best score: " << bestScore << " <=> " << exp(bestScore) << endl;
      cerr << "Using p=" << modelP << endl;
   }

   if (arg.weight_outfile != "") {
     ofstream woutstr;
     woutstr.open(arg.weight_outfile.c_str(), ios_base::app);
     LOG_VERBOSE2(verboseLogger, "Writing best powell weights to file");
     for (Uint i=0; i<powellWeightsOut.size(); i++) {
       woutstr << "BLEU score: " << exp(bestScores[i]);
       for (uVector::const_iterator witr=powellWeightsOut[i].begin(); witr!=powellWeightsOut[i].end(); witr++)
         woutstr << " " << *witr;
       woutstr << endl;
     }
     woutstr.flush();
     woutstr.close();
   }
   LOG_VERBOSE2(verboseLogger, "Writing model to file");
   ffset.write(arg.model_out);
} END_MAIN
