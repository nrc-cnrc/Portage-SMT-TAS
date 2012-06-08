/**
 * @author Aaron Tikuisis
 * @file canoe_constants.cc  This file contains constant values.
 * 
 * $Id$
 * 
 * Canoe Decoder
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "canoe_general.h"

namespace Portage
{
    // INFINITY is now defined in portage_defs.cc
	 	  
    // ALMOST_INFINITY is not really used any more; -ALMOST_INFINITY has been
    // been replaced by LOG_ALMOST_0 (since it is always used as log(almost zero)).
    // EJJ 7Jul2005: changed LOG_ALMOST_0 from -6 (approx 0.0024) to -18
    // (approx 1.5E0-8), a value more likely to be smaller than observed
    // probabilities in our phrase tables, which easily have values around
    // 1E-6 or 1E-7.
    // EJJ 2Feb2006: LOG_ALMOST_0 is also defined in tm/tmtext_filter.pl - if
    // you change it here, change it there too!!!
    const double ALMOST_INFINITY = 100;
    const double LOG_ALMOST_0 = -18;
    
    // Indicates no pruning size limit
    const Uint NO_SIZE_LIMIT = 0;
    
    // Indicates no limit on maximum distortion cost
    const int NO_MAX_DISTORTION = -1;
    
    // Indicates no limit on max levenshtien
    const int NO_MAX_LEVENSHTEIN = -1;

    // Indicates no limit on max itg distortion
    const int NO_MAX_ITG = -1;
} // Portage
