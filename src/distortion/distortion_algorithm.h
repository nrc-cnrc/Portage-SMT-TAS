// $Id$
/**
 * @author Samuel Larkin
 * @file distortion_count.h
 * @brief 
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#ifndef __DISTORTION_ALGORITHM__H__
#define __DISTORTION_ALGORITHM__H__

#include "phrase_pair_extractor.h"
#include "dmstruct.h"     // DistortionCount
#include "phrase_table.h" // PhraseTableGen
#include <string>
#include <vector>

namespace Portage {
   // Grunt work of word-based (Moses-style) LDM counting
   struct word_ldm_count {
      void operator()(
	    const vector<string>& toks1,
	    const vector<string>& toks2,
	    const vector< vector<Uint> >& sets1,
	    PhraseTableGen<DistortionCount>& pt,
	    PhrasePairExtractor& ppe
	    );
   };

   // Grunt work of hierarchical LDM counting (Galley and Manning 2008)
   struct hier_ldm_count{
      void operator()(
	    const vector<string>& toks1,
	    const vector<string>& toks2,
	    const vector< vector<Uint> >& sets1,
	    PhraseTableGen<DistortionCount>& pt,
	    PhrasePairExtractor& ppe
	    );
   };
};

#endif  // __DISTORTION_ALGORITHM__H__

