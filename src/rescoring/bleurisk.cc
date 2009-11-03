/**
 * @author Nicola Ueffing
 * @file bleurisk.cc  Implementation of riskBleu
 * 
 * 
 * COMMENTS: 
 *
 * BLEU-based risk over nbest lists.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "bleu.h"
#include "bleurisk.h"
#include "featurefunction_set.h"
#include <map>

using namespace Portage;

//////////////////////////////////////////////////////////////////////////////
RiskBleu::RiskBleu(const string& args)
: FeatureFunction(args)
{}

bool RiskBleu::parseAndCheckArgs()
{
   string my_args = argument;        // Make a copy to strip parsed parts
   string::size_type pos = 0;

   // lenNbest: number of hypotheses used as basis for risk calculation
   pos = my_args.find("#");
   const string s = my_args.substr(0, pos);
   if (!s.empty()) {
     lenNbest = atoi(s.c_str());
   }
   else {
     error(ETFatal, "You must provide a number of hypotheses used for BLEU risk calculation");
     return false;
   }

   // smoothBleu: Smoothing for sentence-level BLEU calculation
   if (pos != string::npos) {
      my_args = my_args.substr(pos+1);
      pos = my_args.find("#");
      smoothBleu = atoi(my_args.substr(0, pos).c_str());
   }
   else {
     error(ETFatal, "You must provide a smoothing method for BLEU risk calculation, see bleumain -h for help");
     return false;
   }

   // Scale
   if (pos != string::npos) {
      my_args = my_args.substr(pos+1);
      pos = my_args.find("#");
      const string s = my_args.substr(0, pos);
      if (!s.empty()) {
        ff_scale = atof(s.c_str());
      }
      else {
        error(ETFatal, "You must provide a scaling factor for the sentence probabilities in BLEU risk calculation");
        return false;
      }
   }

   // ff_wts_file: Feature function weight file
   if (pos != string::npos) {
      my_args = my_args.substr(pos+1);
      pos = my_args.find("#");
      ff_wts_file = my_args.substr(0, pos);
   }
   else {
      error(ETFatal, "You must provide a weight file");
      return false;
   }

   // Friendly feature_function_tool -check reminder
   // This is not a syntax error, just a friendly warning
   static bool beenWarned = false;
   if (ff_wts_file == "<ffval-wts>") {
      if (!beenWarned) {
         error(ETWarn, "Don't forget to give a canoe.ini to gen-features-parallel.sh");
         beenWarned = true;
      }
   }
   else {
     if (!check_if_exists(ff_wts_file)){
       error(ETFatal, "File is not accessible: %s", ff_wts_file.c_str());
       return false;
     }
   }

   // Prefix
   if (pos != string::npos) {
     ff_prefix = my_args.substr(pos+1);
   }

   return true;
}

void RiskBleu::source(Uint s, const Nbest * const nbest)
{
   FeatureFunction::source(s, nbest);

   scores.clear();
   scores.resize(K);

   FeatureFunctionSet  ffset;
   uVector             weights;
   assert(!ff_wts_file.empty());
   ffset.read(ff_wts_file, true, ff_prefix.c_str(), false);
   ffset.getWeights(weights);
   weights *= ff_scale;

   uMatrix scoreMatrix;
   scoreMatrix.clear();
   Nbest nb = *nbest; // we can't pass a const to computeFFMatrix
   ffset.computeFFMatrix(scoreMatrix, s, nb);
   const uVector sentScores = boost::numeric::ublas::prec_prod(scoreMatrix, weights);

   map<float, Uint> bestScores;
   for (Uint i = 0; i < sentScores.size(); ++i) // put all (score,index) pairs into map
     bestScores.insert(make_pair(-sentScores[i], i));
   
   Uint n=0;
   double sum = 0.0;
   double minCost = bestScores.begin()->first;
   //cerr << "min costs (max.prob.): " << minCost << endl;
   for (map<float, Uint>::const_iterator itr=bestScores.begin();
        itr!=bestScores.end() && ++n<=min(K, lenNbest); itr++) {
     sum += exp(minCost - itr->first);
     //cerr << "sum " << itr->second << ": " << itr->first << "->" << sum << endl;
   }
   double lognorm = minCost - log(sum);
   //cerr << "lognorm: " << lognorm << endl;
   
   // Use each hyp. as 'candidate' in BLEU calculation
   for (Uint i = 0; i < K; ++i) {

     if ((*nbest)[i].getTokens().empty()) {
       scores[i] = 0;
     }
     else {
       
       Tokens hyp  = (*nbest)[i].getTokens();
       double risk = 0;

       /**
          cerr << "===============" << endl
          << "Hyp " << i << ": ";
          for (Tokens::const_iterator itr=hyp.begin(); itr!=hyp.end(); itr++)
          cerr << *itr << " ";
          cerr << endl << endl;
       **/

       vector<Tokens> refs;

       Uint n = 0;
       // Use each of the <lenNbest> top hypotheses as 'reference'
       for (map<float, Uint>::const_iterator j=bestScores.begin(); 
              j!=bestScores.end() && ++n<=min(K, lenNbest); j++) {
              
         if (i != j->second) {
           refs.clear();
           refs.push_back((*nbest)[j->second].getTokens());
           BLEUstats bleu(hyp, refs, smoothBleu);
           risk += exp(lognorm-j->first) * (1.0-exp(bleu.score()));
           
           /**
              cerr << "  Hyp " << j->second << " with score " << j->first << ": ";
              for (Tokens::const_iterator itr=refs[0].begin(); itr!=refs[0].end(); itr++)
              cerr << *itr << " ";
              cerr << endl << "BLEU " << exp(bleu.score()) // << "/" << bleu.score()
              << " norm.costs/prob. " << lognorm-j->first << "/" << exp(lognorm-j->first)
              << " -> risk current/total " << exp(lognorm-j->first) * (1.0-exp(bleu.score())) 
              << " / " << risk << endl << endl;
           **/
         }
       } // for j
       scores[i] = risk;
     } // else (hyp. i is non-empty)
   } // for i
}


