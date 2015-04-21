/**
 * @author Samuel Larkin based on George Foster, based on "rescore" by Aaron Tikuisis
 * @file rescore_translate.cc
 * @brief Pick the best translation for a source given an nbest list a feature
 * functions weights vector.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */


#include "rescore_translate.h"
#include "featurefunction_set.h"
#include "rescore_io.h"

#include "exception_dump.h"
#include "rescoring_general.h"
#include "printCopyright.h"
#include "bleu.h"
#include "translationReader.h"
#include "file_utils.h"

#include <queue>
#include <stack>
#include <sstream>
#include <boost/scoped_ptr.hpp>


using namespace Portage;
using namespace std;

// Assuming v is a vector of logprobs, return log of the sum of the
// probabilities in the vector.
static double logNorm(const uVector& v)
{
   double sum = 0.0;
   double max = 0.0;
   for (Uint i = 0; i < v.size(); ++i)
      if (v[i] < max) max = v[i];
   for (Uint i = 0; i < v.size(); ++i)
      sum += exp(v[i] - max);
   return log(sum) + max;
}

// Write to os the vector of features for the kth hypothesis in feature matrix
// H, containing M features.
static void writeFeatureVector(const uMatrix& H, Uint M, Uint k, ostream& os)
{
   for (Uint m = 0; m < M; ++m) {
      if (m) os << '\t';
      os << (isfinite(H(k,m)) ? H(k,m) : -9999);
   }
}

////////////////////////////////////////////////////////////////////////////////
// main
int MAIN(argc, argv)
{
   printCopyright(2004, "rescore_translate");
   using namespace rescore_translate;
   ARG arg(argc, argv);

   ////////////////////////////////////////
   // SOURCE
   LOG_VERBOSE2(verboseLogger, "Reading source file");
   Sentences  sources;
   const Uint S(RescoreIO::readSource(arg.src_file, sources));
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());

   ////////////////////////////////////////
   // DUMP FF MATRIX MODE
   oMagicStream ffmatrix, nbestmatrix, bleumatrix;
   bool dump_for_mira = false;
   if (!arg.dump_for_mira.empty()) {
      dump_for_mira = true;
      string ffvalsfiles(arg.dump_for_mira + ".allffvals.gz");
      string nbestfile(arg.dump_for_mira + ".allnbest.gz");
      string dummybleufile(arg.dump_for_mira + ".allbleus.gz");
      LOG_VERBOSE2(verboseLogger, "Writing FF values matrix to %s", ffvalsfiles.c_str());
      LOG_VERBOSE2(verboseLogger, "Writing variable length N-best lists to %s", nbestfile.c_str());
      LOG_VERBOSE2(verboseLogger, "Writing dummy allbleus file %s", dummybleufile.c_str());
      ffmatrix.safe_open(ffvalsfiles);
      ffmatrix.precision(9);
      nbestmatrix.safe_open(nbestfile);
      bleumatrix.safe_open(dummybleufile);
   }

   ////////////////////////////////////////
   // FEATURE FUNCTION SET
   LOG_VERBOSE2(verboseLogger, "Reading ffset");
   FeatureFunctionSet ffset(dump_for_mira);
   ffset.read(arg.model, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic);
   ffset.createTgtVocab(sources, FileReader::create<Translation>(arg.nbest_file, arg.K));
   uVector wts(ffset.M());
   ffset.getWeights(wts);
   {
      ostringstream oss;
      oss << wts;
      LOG_VERBOSE2(verboseLogger, "Rescoring using weights: %s", oss.str().c_str());
   }

   LOG_VERBOSE2(verboseLogger, "Initializing FF matrix with %d source sentences", S);
   ffset.initFFMatrix(sources);


   ////////////////////////////////////////
   // Create a NBest reader that will also read alignments if needed.
   LOG_VERBOSE2(verboseLogger, "Creating NBest reader");
   NbestReader  nbestReader(FileReader::createT(arg.nbest_file, arg.alignment_file, arg.K));

   if (ffset.requires() & FF_NEEDS_ALIGNMENT) {
      if (arg.alignment_file.empty())
         error(ETFatal, "At least one of feature function requires the alignment");
   }

   boost::scoped_ptr<istream> co_in;
   boost::scoped_ptr<ostream> co_out;
   if (arg.cofile != "") {
      co_in.reset(new iSafeMagicStream(arg.cofile));
      co_out.reset(new oSafeMagicStream(addExtension(arg.cofile, ".resc")));
   }
   vector<string> colines(arg.K);
   boost::scoped_ptr<ostream> fv_out;
   if (arg.fvfile != "")
      fv_out.reset(new oSafeMagicStream(arg.fvfile));
   boost::scoped_ptr<ostream> sc_out;
   if (arg.scfile != "")
      sc_out.reset(new oSafeMagicStream(arg.scfile));

   LOG_VERBOSE2(verboseLogger, "Processing Nbest lists");

   if (arg.bVerbose) {
      if (arg.bMbr)
         cerr << "Minimum Bayes risk (MBR) rescoring, based on BLEU, only 1-best output!" << endl;
      else
         cerr << "Maximum a-posteriori (MAP) rescoring (default behaviour)" << endl;
   }

   Uint s(0);
   for (; nbestReader->pollable(); ++s)
   {
      Nbest  nbest;
      nbestReader->poll(nbest);
      Uint K(nbest.size());

      LOG_VERBOSE3(verboseLogger, "Computing FF matrix");
      uMatrix H;
      ffset.computeFFMatrix(H, s, nbest);
      if (dump_for_mira) {
         static Uint M = ffset.M(); // number of features
         for (Uint k = 0; k < nbest.size(); ++k) {
            ffmatrix << s << '\t';
            writeFeatureVector(H, M, k, ffmatrix);
            ffmatrix << nf_endl;
            nbestmatrix << s << '\t' << nbest[k] << nf_endl;
            bleumatrix << s << ' ' << 0 << nf_endl;
         }
         continue;
      }

      K = nbest.size();  // IMPORTANT K might change if computeFFMatrix detects empty lines

      if (co_in) {
         for (Uint i = 0; i < colines.size(); ++i)
            if (!getline(*co_in, colines[i]))
               error(ETFatal, "file %s too short", arg.cofile.c_str());
      }

      if (!arg.bMbr) { // maximum a-posteriori rescoring
         if (arg.kout == 1) {      // output single best hypothesis
            if (K==0) {
               cout << endl;
               if (co_out) (*co_out) << endl;
               if (fv_out) (*fv_out) << nf_endl;
               if (sc_out) (*sc_out) << nf_endl;
            } else {
               const uVector Scores = boost::numeric::ublas::prec_prod(H, wts);
               const double logz = arg.conf_scores ? logNorm(Scores) : 0.0;
               const Uint bestIndex = my_vector_max_index(Scores);         // k = sentence which is scored highest
               if (arg.bPrintRank) cout << bestIndex+1 << " ";
               if (arg.print_scores) cout << Scores[bestIndex] - logz << " ";
               cout << nbest.at(bestIndex) << endl;  // DISPLAY best hypothesis
               if (co_out) (*co_out) << colines[bestIndex] << endl;
               if (fv_out) {
                  static Uint M = ffset.M();
                  writeFeatureVector(H, M, bestIndex, (*fv_out));
                  (*fv_out) << nf_endl;
               }
               if (sc_out) (*sc_out) << Scores[bestIndex] - logz << nf_endl;
            }
         } else {                  // ouptput k best hypotheses
            const uVector Scores = boost::numeric::ublas::prec_prod(H, wts);
            const double logz = arg.conf_scores ? logNorm(Scores) : 0.0;
            priority_queue< pair<float, Uint> > best_scores;
            Uint kout = (arg.kout == 0) ? K : min(arg.kout, K);
            for (Uint i = 0; i < Scores.size(); ++i) { // put best kout into pq
               best_scores.push(make_pair(-Scores[i], i));
               if (best_scores.size() > kout) best_scores.pop();
            }
            stack < pair<float, Uint> > best_scores_stack;
            while (!best_scores.empty()) { // reverse order of queue
               best_scores_stack.push(best_scores.top());
               best_scores.pop();
            }
            while (!best_scores_stack.empty()) { // output
               pair<float, Uint>& p = best_scores_stack.top();
               if (arg.bPrintRank) cout << p.second+1 << " ";
               if (arg.print_scores) cout << Scores[p.second] - logz << " ";
               cout << nbest.at(p.second) << endl;  // DISPLAY best hypothesis
               if (co_out) (*co_out) << colines[p.second] << endl;
               if (fv_out) {
                  static Uint M = ffset.M();
                  writeFeatureVector(H, M, p.second, (*fv_out));
                  (*fv_out) << nf_endl;
               }
               if (sc_out) (*sc_out) << Scores[p.second] - logz << nf_endl;
               best_scores_stack.pop();
            }
            if (arg.K)             // pad with blank lines if necessary
               for (; kout < arg.kout; ++kout) {
                  cout << endl;
                  if (co_out) (*co_out) << endl;
                  if (fv_out) (*fv_out) << nf_endl;
                  if (sc_out) (*sc_out) << nf_endl;
               }
         }
      } else { // Minimum Bayes risk rescoring using BLEU as loss function

        // Sort hypotheses by sentence score
        const uVector Scores = boost::numeric::ublas::prec_prod(H, wts) * arg.glob_scale;
        if (arg.bVerbose) cerr << "Scaling all sentence scores with " << arg.glob_scale << endl;

        map<float, Uint> best_scores;
        for (Uint i = 0; i < Scores.size(); ++i) // put all (score,index) pairs into map
          best_scores.insert(make_pair(-Scores[i], i));

        // Consider only the <kmbr> top hypotheses for MBR
        const Uint kmbr = arg.kmbr == 0 ? K : min(arg.kmbr, K);

        // Determine total prob. mass of op <kmbr> hypotheses for normalization
        Uint n=0;
        double sum = 0.0;
        const double minCost = best_scores.begin()->first;
        if (arg.bVerbose) cerr << "min costs (max.prob.): " << minCost << endl;
        for (map<float, Uint>::const_iterator itr=best_scores.begin();
             itr!=best_scores.end() && ++n<=kmbr; itr++) {
          sum += exp(minCost - itr->first);
          if (arg.bVerbose) cerr << "sum " << itr->second << ": " << itr->first << "->" << sum << endl;
        }
        const double lognorm = minCost - log(sum);
        if (arg.bVerbose)
          cerr << "==============================" << endl
               << "Normalizing everything by logprob. " << lognorm << endl;

        // Determine MBR hypotheses based on BLEU
        if (K==0)
          cout << endl;

        priority_queue< pair<float, Uint> > minRisks;

        double minRisk = INFINITY;
        Uint bestIndex = K+1;
        Uint n1=0;

        // Use each hyp. as 'candidate' in BLEU calculation
        for (map<float, Uint>::const_iterator i1=best_scores.begin();
             i1!=best_scores.end() && ++n1<=kmbr; i1++) {
          Tokens hyp1 = nbest[i1->second].getTokens();
          double currentRisk = 0;
          if (arg.bVerbose) {
            cerr << "===============" << endl
                 << "Hyp " << i1->second << " with score " << i1->first << ": ";
            for (Tokens::const_iterator itr=hyp1.begin(); itr!=hyp1.end(); itr++)
              cerr << *itr << " ";
            cerr << endl << endl;
          }
          Uint n2 = 0;
          // Use each other hyp. as 'reference'
          for (map<float, Uint>::const_iterator i2=best_scores.begin();
               i2!=best_scores.end() && ++n2<=kmbr; i2++) {

            if (i1->second != i2->second) {
              vector<Tokens> refs;
              refs.push_back(nbest[i2->second].getTokens());
              BLEUstats bleu(hyp1, refs, arg.smooth_bleu);
              currentRisk += exp(lognorm-i2->first) * (1.0-exp(bleu.score()));

              if (arg.bVerbose) {
                cerr << "  Hyp " << i2->second << " with score " << i2->first << ": ";
                for (Tokens::const_iterator itr=refs[0].begin(); itr!=refs[0].end(); itr++)
                  cerr << *itr << " ";
                cerr << endl << "BLEU " << exp(bleu.score())
                     << " norm.costs/prob. " << lognorm-i2->first << "/" << exp(lognorm-i2->first)
                     << " -> current/summed risk " << exp(lognorm-i2->first) * (1.0-exp(bleu.score()))
                     << " / " << currentRisk << endl << endl;
              }
              if (currentRisk > minRisk)
                break;
            }
          } // for i2
          if (currentRisk < minRisk) {
            minRisk = currentRisk;
            bestIndex = i1->second;
            if (arg.bVerbose)
              cerr << "minimal risk so far: " << minRisk << endl;
          }
        } // for i1
        // output
        if (arg.bPrintRank) cout << bestIndex+1 << " ";
        if (arg.print_scores) cout << minRisk << " ";
        cout << nbest.at(bestIndex) << endl;  // DISPLAY best hypothesis

      } // else (MBR)
   }

   nbestReader.reset();
   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);

} END_MAIN
