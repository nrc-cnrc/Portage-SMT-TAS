/**
 * @author George Foster
 * @file portage_api.h  Portage API
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

#ifndef PORTAGE_API_H
#define PORTAGE_API_H

#include <string>
#include "basicmodel.h"
#include "inputparser.h"
#include "translator_if.h"
#include "preprocessor.h"
#include "postprocessor.h"
#include "config_io.h"

namespace Portage {

/// Concrete implementation of a translator
class PortageAPI: public Translator_if
{
#ifndef PORTAGE_ECHO_PADDLE
   Preprocessor  prep;  ///< Preprocessing helper
   Postprocessor post;  ///< Postprocessing helper
#endif
   CanoeConfig c;  ///< canoe's configuration options

   BasicModelGenerator* bmg;  ///< Decoder

   /// Contains the tokenized source sentences.
   vector<vector<string> >    src_sents;
   /// Contains the tokenized translation sentences.
   vector<vector<string> >    tgt_sents;
   /// Contains the current marked sentence we are translating
   vector<MarkedTranslation>  cur_mark;

public:
   /**
    * Construct. Load time may be significant.
    * @param srclang source language for translation, one of "en", "fr", or "ch"
    * @param tgtlang target language
    * @param config base name for canoe configuration; the file
    * "<config>.<srclang>2<tgtlang>" must exist at $PORTAGE/models/demo.
    * @param models_in_demo_dir if true, all models in config file are assumed
    * to exist in demo dir; otherwise, model names must be valid paths (either
    * absolute or relative to cwd).
    * @param verbose  Should we display verbose as we go?
    */       
   PortageAPI(const string& srclang, const string& tgtlang, const string& config, 
	      bool models_in_demo_dir, bool verbose = false);

   /// Destructor
   virtual ~PortageAPI() {delete bmg;}
   
   /**
    * Translate a given source text.
    * @param source_text A plain text string, with no markup and no
    * linguistic processing, eg "This is well-formed input. Translate it!"
    * @param target_text The generated translation.
    */
   virtual void translate(const string& source_text, string& target_text);
   
}; //ends PortageAPI

} // ends namespace Portage

#endif
