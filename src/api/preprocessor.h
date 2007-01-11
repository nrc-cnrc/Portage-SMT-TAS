/**
 * @author George Foster
 * @file preprocessor.h  Prepares raw source sentences for translation.
 * 
 * 
 * COMMENTS: 
 *
 * Don't forget to add rule-based translation.
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <vector>
#include <portage_defs.h>
#include <errors.h>

namespace Portage {

/**
 * Prepares a raw source sentence for translation.
 */
class Preprocessor
{
   /// A temporary file to put the tokenized and lowercased version of the source sentences
   string tempfile;
   /// Command string for a system call to tokenize and lowercase the source sentences
   string cmd;

public:

   /**
    * Construct.
    * @param srclang source language code for translation, one of "en", "fr",
    * "ch"
    */       
   Preprocessor(const string& srclang);

   /// Destructor.
   ~Preprocessor() {}
   
   /**
    * Preprocess a source text.
    * @param raw_text A plain text string, with no markup and no
    * linguistic processing, eg "This is well-formed input. Translate it!"
    * @param prep_text Text divided into sentences, each of which is a
    * sequence of tokens. Eg:
    * this is well-formed input .
    * translate it !
    */
   void proc(const string& raw_text, vector<vector<string> >& prep_text);
};

}

#endif
