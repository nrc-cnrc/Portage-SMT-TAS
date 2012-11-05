/**
 * @author Samuel Larkin
 * @file test_Giza2AlignmentFile.h
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "ibm.h"
#include <sstream>

using namespace Portage;

namespace Portage {

class TestGiza2AlignmentFile : public CxxTest::TestSuite 
{
public:
   void testSimple() {
      using namespace std;
      stringstream* ss = new stringstream;
      *ss << "# Sentence pair (1) source length 2 target length 3 alignment score : 3.63487e-24" << endl;
      *ss << "le chat x" << endl;
      *ss << "NULL ({ 3 }) the ({ 1 }) cat ({ 2 })" << endl;
      Giza2AlignmentFile aligner(ss);
      vector<string> source;
      split("the cat", source);
      vector<string> target;
      split("le chat T", target);
      vector<Uint> tgt_al;
      aligner.align(source, target, tgt_al);

      TS_ASSERT_EQUALS(3, tgt_al.size());
      Uint ref[] = { 0, 1 };
      TS_ASSERT_SAME_DATA(&tgt_al[0], ref, tgt_al.size()+sizeof(Uint));
   }

   void testRead() {
      using namespace std;
      stringstream* ss = new stringstream;
      *ss << "# Sentence pair (1) source length 11 target length 16 alignment score : 3.63487e-24" << endl;
      *ss << "m. hulchanski est l' un des plus éminents spécialistes du canada en matière de logement ." << endl;
      *ss << "NULL ({ 14 }) dr. ({ 1 }) hulchanski ({ 2 }) is ({ 3 }) one ({ 4 5 }) of ({ 6 }) canada ({ 11 }) 's ({ }) foremost ({ 7 8 9 10 12 13 }) housing ({ 15 }) experts ({ }) . ({ 16 })" << endl;
      Giza2AlignmentFile aligner(ss);
      vector<string> source;
      split("dr. hulchanski is one of canada 's foremost housing experts .", source);
      vector<string> target;
      split("m. hulchanski est l' un des plus éminents spécialistes du canada en matière de logement .", target);
      vector<Uint> tgt_al;
      aligner.align(source, target, tgt_al);

      TS_ASSERT_EQUALS(16, tgt_al.size());
      Uint ref[] = { 0, 1, 2, 3, 3, 4, 7, 7, 7, 7, 5, 7, 7, 0, 8, 10 };
      TS_ASSERT_SAME_DATA(&tgt_al[0], ref, tgt_al.size()+sizeof(Uint));
   }
}; // TestGiza2AlignmentFile

} // Portage
