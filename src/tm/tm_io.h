/**
 * @author George Foster
 * @file tm_io.h  Left-over utilities for translation model I/O, mostly
 *                replaced by utils/str_utils.
 * 
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef TM_IO_H
#define TM_IO_H

#include <string>
#include <vector>
#include <portage_defs.h>

namespace Portage {

/// Left-over utilities for translation model I/O, mostly replaced by
/// utils/str_utils.
namespace TMIO {

/**
 * Join tokens into a single tokenized line.
 * @param tokens to join
 * @param line of text
 * @param bracket put brackets around tokens if true
 * @return "<line>"
 */
string& joinTokens(const vector<string>& tokens, string& line, bool bracket=false);

}
}

#endif
