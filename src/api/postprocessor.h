/**
 * @author George Foster
 * @file postprocessor.h  Helper that does truecasing on a translation.
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include <string>
#include <vector>
#include <portage_defs.h>
#include <errors.h>

namespace Portage {

/// Prepares a translation for output (truecasing)
class Postprocessor
{
   // For licensing reasons, the truecasing engine cannot be supplied with this
   // distribution.  To integrate your truecasing engine with this demo, add it
   // to Postprocessor::proc().
   // Nov 2009: we now have our own truecasing engine, but this API is pretty
   // much obsolete, so we have no plans to integrate it.  The new methodology
   // is based around the translate.sh script in the framework.
   //TrueCaseEngine tc;
   
   bool verbose;  ///< Are we required to print verbose.

   /// A temporary file to put the tokenized and lowercased version of the source sentences.
   string tempfile;
   /// Command string for a system call to tokenize and lowercase the source sentences.
   string cmd;

   /// Joined tokens of split sentences.
   vector<string> sents;

   /// try to make output a bit prettier: remove single quotes and parens
   /// @param sent  sentence to twiddle
   void twiddle(vector<string>& sent);

public:

   /**
    * Construct.
    * @param tgtlang target language code, one of "en", "fr"
    * @param verbose Indicates if we should display verbose
    */       
   Postprocessor(const string& tgtlang, bool verbose = false);

   /// Destructor.
   ~Postprocessor() {}
   
   /**
    * Postprocess an output text.
    * @param mt_out MT output in the form of tokenized sentences.
    * @param processed_text Upper-case, detokenized version of raw_text.
    * @return Returns processed_text
    */
   string& proc(vector<vector<string> >& mt_out, string& processed_text);
}; // ends Postprocessor

} // ends namespace Portage

#endif
