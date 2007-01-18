/**
 * @author George Foster
 * @file rescore_io.cc  Implementation of rescoring utilities.
 *
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "rescore_io.h"
#include <file_utils.h>
#include <errors.h>
#include <str_utils.h>

// Required to have proper documentation in doxygen
namespace Portage {
namespace RescoreIO {

Uint readAlignments(const string &filename, vector<Alignment> &alignments) {
  IMagicStream in(filename);
  Alignment a;
  while (a.read(in))
    alignments.push_back(a);
  return alignments.size();
}

void tokenize(const char* sent, vector<string>& toks)
{
   toks.clear();
   string ss(sent);
   split(ss, toks);
}

Uint readSource(const string& filename, Sentences& sentences)
{
   IMagicStream istr(filename);

   string line;
   while (getline(istr, line)) {
      sentences.push_back(line);
   }
   return sentences.size();
}

} // ends namespace RescoreIO
} // ends namespace Portage
