/**
 * @author Samuel Larkin
 * @file test_feature_function_grammar.h  Test suite for feature function grammar
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "feature_function_grammar.h"

using namespace Portage;
using namespace boost::spirit;

namespace Portage {

class TestFeatureFunctionGrammar : public CxxTest::TestSuite 
{
public:
   void testGoodSyntaxForTraining() {
      // Different syntaxes for a feature function in a rescoring-model
      // Features WITHOUT a distribution
      const char* const src [] = {
         "File2FF" ,
         "File2FF:somefile.../argument,1",
         "nbestNgramPost:1#1",
         "FileFF:ff.nbestNgramPost.1#1",
         "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch",
         "File2FF N(6.0,1.0)" ,
         "File2FF U(1.0,11.0)" ,
         "File2FF:somefile.../argument,1 N(6.0,1.0)",
         "File2FF:somefile.../argument,1 U(1.0,11.0)",
         "nbestNgramPost:1#1 N(-5,0.04)",
         "nbestNgramPost:1#1 U(-5,-3)",
         "FileFF:ff.nbestNgramPost.1#1 N(-5,0.04)",
         "FileFF:ff.nbestNgramPost.1#1 U(-5,-3)",
         "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch N(3, 0.2)",
         "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch U(-4.5, 12.452)"
         };
      // During training we either have no fix weights or we have a
      // Uniform/Normal distribution.
      parseGoodSyntax(src, true);
   }
   void testGoodSyntaxForDecoding() {
      // Different syntaxes for a feature function in a rescoring-model
      // Features WITH a distribution (Fix|Uniform|Normal)
      const char* const src [] = {
         "File2FF -0.32143",
         "File2FF N(6.0,1.0)" ,
         "File2FF U(1.0,11.0)" ,
         "File2FF:somefile.../argument,1 0.343" ,
         "File2FF:somefile.../argument,1 N(6.0,1.0)",
         "File2FF:somefile.../argument,1 U(1.0,11.0)",
         "nbestNgramPost:1#1 3",
         "nbestNgramPost:1#1 N(-5,0.04)",
         "nbestNgramPost:1#1 U(-5,-3)",
         "FileFF:ff.nbestNgramPost.1#1 3",
         "FileFF:ff.nbestNgramPost.1#1 N(-5,0.04)",
         "FileFF:ff.nbestNgramPost.1#1 U(-5,-3)",
         "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch -9",
         "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch N(1, 0.2)",
         "FileFF:ff.HMMTgtGivenSrc.hmm.en_given_ch U(-4.5, 12.452)"
         };
      // When decoding, we must have a distrubution (Fix|Uniform|Normal).
      parseGoodSyntax(src, false);
    }
    void parseGoodSyntax(const char* const src[], bool training) {
      for (unsigned int i(0); i<sizeof(src)/sizeof(const char* const); ++i) {
         Portage::feature_function_grammar gram(training);
         TS_ASSERT(parse(src[i], gram, space_p).full);
      }
   }
}; // TestFeatureFunctionGrammar

} // Portage
