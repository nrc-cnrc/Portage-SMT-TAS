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

#include <portage_defs.h>
#include <basic_data_structure.h>
#include <string>
#include <vector>


namespace Portage {

/// Grouping all related functions in a namespace.
/// Prevents pollution of the global namespace.
namespace RescoreIO {

   /**
    * Reads all alignments in a file.
    * @param filename  file containing the alignments to read.
    * @param a         returned alignments.
    * @return Returns the number of read alignments.
    */
   Uint readAlignments(const string &filename, vector<Alignment> &a);

   /**
    * Tokenizes a sentence.
    * @param sent  sentence to tokenize.
    * @param toks  returned tokenized sent.
    */
   void tokenize(const char* sent, vector<string>& toks);

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
