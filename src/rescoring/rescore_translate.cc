/**
 * @author Samuel Larkin based on George Foster, based on "rescore" by Aaron Tikuisis
 * @file rescore_translate.cc  Program rescore_translate which picks the best
 * translation for a source given an nbest list a feature functions weights
 * vector.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */


#include <rescore_translate.h>
#include <boostDef.h>
#include <featurefunction.h>
#include <rescore_io.h>
#include <bleu.h>
#include <basic_data_structure.h>
#include <exception_dump.h>
#include <fileReader.h>
#include <rescoring_general.h>
#include <printCopyright.h>


using namespace Portage;
using namespace std;


////////////////////////////////////////////////////////////////////////////////
// main
int MAIN(argc, argv)
{
   printCopyright(2004, "rescore_translate");
   using namespace rescore_translate;
   ARG arg(argc, argv);

   LOG_VERBOSE2(verboseLogger, "Reading ffset");
   FeatureFunctionSet ffset;
   ffset.read(arg.model, arg.bVerbose, arg.ff_pref.c_str(), arg.bIsDynamic);
   uVector ModelP(ffset.M());
   ffset.getWeights(ModelP);


   ////////////////////////////////////////
   // SOURCE
   LOG_VERBOSE2(verboseLogger, "Reading source file");
   Sentences  sources;
   const Uint S(RescoreIO::readSource(arg.src_file, sources));
   if (S == 0) error(ETFatal, "empty source file: %s", arg.src_file.c_str());


   ////////////////////////////////////////
   // ALIGNMENT
   ifstream astr;
   if (arg.bReadAlignments) {
      LOG_VERBOSE2(verboseLogger, "Reading alignments from %s", arg.alignment_file.c_str());
      astr.open(arg.alignment_file.c_str());
      if (!astr) error(ETFatal, "unable to open alignment file %s", arg.alignment_file.c_str());
   }

   LOG_VERBOSE2(verboseLogger, "Processing Nbest lists");
   NbestReader  pfrN(FileReader::create<Translation>(arg.nbest_file, S, arg.K));
   Uint s(0);
   for (; pfrN->pollable(); ++s)
   {
      Nbest  nbest;
      pfrN->poll(nbest);
      Uint K(nbest.size());

      vector<Alignment> alignments(K);
      Uint k(0);
      for (; arg.bReadAlignments && k < K && alignments[k].read(astr); ++k)
      {
         nbest[k].alignment = &alignments[k];
      }
      if (arg.bReadAlignments && (k != K)) error(ETFatal, "unexpected end of nbests file after %d lines (expected %dx%d=%d lines)", s*K+k, S, K, S*K);

      LOG_VERBOSE3(verboseLogger, "Initializing FF matrix");
      ffset.initFFMatrix(sources, K);

      LOG_VERBOSE3(verboseLogger, "Computing FF matrix");
      uMatrix H;
      ffset.computeFFMatrix(H, s, nbest);
      K = nbest.size();  // IMPORTANT K might change if computeFFMatrix detects empty lines

      if (K==0) {
	  cout << endl;
      } 
      else {

	  const uVector Scores = boost::numeric::ublas::prec_prod(H, ModelP);
	  const Uint bestIndex = my_vector_max_index(Scores);         // k = sentence which is scored highest
	  cout << nbest.at(bestIndex) << endl;  // DISPLAY best hypothesis
      }
   }

   astr.close();
   if (s != S) error(ETFatal, "File inconsistency s=%d, S=%d", s, S);

} END_MAIN
