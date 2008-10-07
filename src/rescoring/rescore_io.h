/**
 * @author George Foster
 * @file rescore_io.h  Utilities for reading sources and alignments.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef RESCORE_IO_H
#define RESCORE_IO_H

#include "basic_data_structure.h"

namespace Portage {

/// Grouping all related functions in a namespace.
/// Prevents pollution of the global namespace.
/// Most methods have been removed from here because they were obsolete.
namespace RescoreIO {

   /**
    * Reads all sentences in a file.
    * @param filename   file containing the sentences.\
    * @param sentences  returned sentences (vector).
    * @return Returns the number of read sentences.
    */
   Uint readSource(const string& filename, Sentences& sentences);

} // ends namespace RescoreIO

} // ends namespace Portage

#endif
